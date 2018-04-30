/*
 * tunfolder.cc
 *
 *  Created on: Apr 27, 2018
 *      Author: admin
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>

#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/Format.h>

#include "stid/executor.hh"
#include "pes/process.hh"
#include "unfolder/tunfolder.hh" // must be before verbosity.h

#include "misc.hh"
#include "verbosity.h"
#include "opts.hh"

namespace dpu
{

Tunfolder::Tunfolder (const stid::ExecutorConfig &config) :
   _exec (nullptr),
   _m (nullptr),
   _config (config)
{
   unsigned i;

//   // initialize the start array (this is an invariant expected and maintained
//   // by stream_to_events)
//   for (i = 0; i < Unfolding::MAX_PROC; i++)
//      StreamConverter<T>::start[i] = nullptr;
}

Tunfolder:: Tunfolder (const Tunfolder &other) :
      _exec (other._exec),
//      _context (other._context), // Load_bitcode se set up context
      _path (other._path),
      _m (other._m),
      _config (other._config)
{
   PRINT ("Tunfolder.cctor");
}

Tunfolder::~Tunfolder ()
{
   DEBUG ("unf.dtor: this %p", this);
   delete _exec;
}

void Tunfolder::_load_bitcode (std::string &&filepath)
{
   llvm::SMDiagnostic err;
   std::string errors;

   ASSERT (filepath.size());
   ASSERT (_path.size() == 0);
   ASSERT (_exec == 0);
   ASSERT (_m == 0);
   _path = std::move (filepath);

   // necessary for the JIT engine; we should move this elsewhere
   static bool init = false;
   if (not init)
   {
      init = true;
      llvm::InitializeNativeTarget();
      llvm::InitializeNativeTargetAsmPrinter();
      llvm::InitializeNativeTargetAsmParser();
   }

   // parse the .ll file and get a Module out of it
   PRINT ("dpu: unf: loading bitcode");
   std::unique_ptr<llvm::Module> mod (llvm::parseIRFile (_path, err, _context));
   _m = mod.get();

   // if errors found, report and terminate
   if (! mod.get ()) {
      llvm::raw_string_ostream os (errors);
      err.print (_path.c_str(), os);
      os.flush ();
      DEBUG ("unf: unf: load-bytecode: '%s': %s\n", _path.c_str(), errors.c_str());
      throw std::invalid_argument (errors);
   }

   // print external symbols
//   if (verb_trace) print_external_syms ("dpu: "); // Se xem xet sau

//   PRINT ("dpu: unf: O%u-optimization + jitting...", opts::optlevel); // Cai nay thuoc ve C15
   DEBUG ("unf: unf: load-bytecode: setting up the bytecode executor...");
   try {
      _exec = new stid::Executor (std::move (mod), _config);
   } catch (const std::exception &e) {
      DEBUG ("unf: unf: load-bytecode: errors preparing the bytecode executor");
      DEBUG ("unf: unf: load-bytecode: %s", e.what());
      throw e;
   }
   DEBUG ("unf: unf: load-bytecode: executor successfully created!");

   if (opts::instpath.size())  // Khong co opts o day? Phai xem xet.
   {
      TRACE ("dpu: unf: load-bytecode: saving instrumented bytecode to %s",
         opts::instpath.c_str());
      _store_bitcode (opts::instpath.c_str());
   }

   DEBUG ("dpu: unf: load-bytecode: done!");
}

void Tunfolder::_store_bitcode (const std::string &filename) const
{
   int fd = open (filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
   ASSERT (fd >= 0);
   llvm::raw_fd_ostream f (fd, true);
   f << *_m;
}

void Tunfolder::_print_external_syms (const char *prefix)
{
   std::string str;
   llvm::raw_string_ostream os (str);
   std::vector<std::pair<llvm::StringRef,llvm::Type*>> funs;
   std::vector<std::pair<llvm::StringRef,llvm::Type*>> globs;
   size_t len;

   ASSERT (m);
   if (! prefix) prefix = "";

   // functions
   len = 0;
   for (llvm::Function &f : _m->functions())
   {
      if (not f.isDeclaration()) continue;
      funs.emplace_back (f.getName(), f.getType());
      if (len < funs.back().first.size())
         len = funs.back().first.size();
   }
   std::sort (funs.begin(), funs.end());
   os << prefix << "External functions:\n";
   for (auto &pair : funs)
   {
      os << prefix << llvm::format ("  %-*s ", len, pair.first.str().c_str());
      os << *pair.second << "\n";
   }

   // global variables
   len = 0;
   for (llvm::GlobalVariable &g : _m->globals())
   {
      if (not g.isDeclaration()) continue;
      globs.emplace_back (g.getName(), g.getType());
      if (len < globs.back().first.size())
         len = globs.back().first.size();
   }
   std::sort (globs.begin(), globs.end());
   os << prefix << "External variables:\n";
   for (auto &pair : globs)
   {
      os << prefix << llvm::format ("  %-*s ", len, pair.first.str().c_str());
      os << *pair.second << "\n";
   }
   os.flush ();
   PRINT ("%s", str.c_str());
}

void Tunfolder::_set_args (std::vector<const char *> argv)
{
   DEBUG ("unf: set-args: |argv| %zu", argv.size());
   _exec->argv = argv;
}

void Tunfolder::_set_env (std::vector<const char *> env)
{
   if (env.empty() or env.back() != nullptr)
      env.push_back (nullptr);
   DEBUG ("unf: set-env: |env| %zu", env.size());
   _exec->environ = env;
}

void Tunfolder::_set_default_environment ()
{
   char * const * v;
   std::vector<const char *> env;

   // make a copy of our environment
   for (v = environ; *v; ++v) env.push_back (*v);
   env.push_back (nullptr);
   DEBUG ("unf: set-env: |env| %zu", env.size());
   _exec->environ = env;
}

// Ham nay de lam gi nhi? Tai sao ko có stream_to_events???
//Config Tunfolder::_add_one_run (const stid::Replay &r)
//{
//   Config c (Unfolding::MAX_PROC);
//
//   ASSERT (_exec);
//
//   // run the guest
//   DEBUG ("unf: add-1-run: this %p |replay| %zu", this, r.size());
//   _exec->set_replay (r);
//   DEBUG ("unf: add-1-run: running the guest ...");
//   _exec->run ();
//
//   // get a stream object from the executor and transform it into events
//   stid::action_streamt actions (_exec->get_trace ());
//   //actions.print ();
//   //actions.print ();
//   StreamConverter<T>::convert (actions, c);
//   return c;
//}

bool Tunfolder::_is_conflict_free (const std::vector<Event *> &sol,
   const Event *e) const
{
   // we return false iff e is in conflict with some event of the partial solution "sol"
   int i;
   for (i = 0; i < sol.size(); i++)
      if (e->in_cfl_with (sol[i])) return false;
   return true;
}

} // namespace dpu





