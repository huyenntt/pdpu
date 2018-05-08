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
   DEBUG ("Tunf: ctor");
   DEBUG ("Tunf: config: %zu", _config.memsize);
//   unsigned i;

//   // initialize the start array (this is an invariant expected and maintained
//   // by stream_to_events)
//   for (i = 0; i < Unfolding::MAX_PROC; i++)
//      StreamConverter<T>::start[i] = nullptr; // Phai xem lai phan nay, no co can thiet cho unfolding trong c15 hay ko?
}

//Tunfolder:: Tunfolder (const Tunfolder &&other) :
//      _exec (std::move(other._exec)),
//      _context (std::move(other._context)), // Load_bitcode se set up context
//      _path (std::move(other._path)),
//      _m (std::move(other._m)),
//      _config (std::move(other._config))
//{
//   PRINT ("Tunf.cctor");
//   DEBUG ("Tunf: config: %zu", _config.memsize);
//}

//Tunfolder:: Tunfolder (const Tunfolder &other) :
//      _exec (other._exec),
//      _context (other._context), // Load_bitcode se set up context
//      _path (other._path),
//      _m (other._m),
//      _config (other._config)
//{
//   PRINT ("Tunf.cctor");
//   DEBUG ("Tunf: config: %zu", _config.memsize);
//}

Tunfolder::~Tunfolder ()
{
   DEBUG ("tunf.dtor: this %p", this);
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
//   _path = std::move (filepath);
   _path = filepath;
   PRINT ("tunf: loadbitcode: path: %s, filepath %s", _path.c_str(), filepath.c_str());

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
   std::unique_ptr<llvm::Module> mod (llvm::parseIRFile (_path, err, _context));
   _m = mod.get();

   // if errors found, report and terminate
   if (! mod.get ()) {
      llvm::raw_string_ostream os (errors);
      err.print (_path.c_str(), os);
      os.flush ();
      PRINT ("tunf: load-bytecode: '%s': %s\n", _path.c_str(), errors.c_str());
      throw std::invalid_argument (errors);
   }

   // print external symbols
//   if (verb_trace) print_external_syms ("dpu: "); // Se xem xet sau

//   PRINT ("dpu: unf: O%u-optimization + jitting...", opts::optlevel); // Cai nay thuoc ve C15
   PRINT ("tunf: load-bytecode: setting up the bytecode executor...");
   try {
      _exec = new stid::Executor (std::move (mod), _config);
   } catch (const std::exception &e) {
      PRINT ("tunf: load-bytecode: errors preparing the bytecode executor");
      PRINT ("tunf: load-bytecode: %s", e.what());
      throw e;
   }
   PRINT ("tunf: load-bytecode: executor successfully created!");

//   PRINT ("instpath.size: %lu", opts::instpath.size());
   // Thuc ra cai nay store_bitcode chi can thiet khi minh muon luu lai bitcode (co le de backup)
   if (opts::instpath.size())  // Opts trong namespace opts in opts.hh
   {
      PRINT ("tunf: load-bytecode: saving instrumented bytecode to %s",
         opts::instpath.c_str());
      _store_bitcode (opts::instpath.c_str()); // Neu can thi chua bitcode bang function nay
   }

   PRINT ("tunf: load-bytecode: Done!");
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

   ASSERT (_m);
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
   PRINT ("tunf: set-args: |argv| %zu", argv.size());
   _exec->argv = argv;
}

void Tunfolder::_set_env (std::vector<const char *> env)
{
   if (env.empty() or env.back() != nullptr)
      env.push_back (nullptr);
   PRINT ("tunf: set-env: |env| %zu", env.size());
   _exec->environ = env;
}

void Tunfolder::_set_default_environment ()
{
   char * const * v;
   std::vector<const char *> env;

   // make a copy of our environment
   for (v = environ; *v; ++v) env.push_back (*v); // environ la cai gi???
   env.push_back (nullptr);
//   PRINT ("unf: set-env: |env| %zu", env.size());
   _exec->environ = env;
   PRINT ("tunf: set-env: |env| %zu", _exec->environ.size());
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

void  Tunfolder:: _set_replay_sleepset (Replay &replay, const Disset &d, const Cut &j)
{
     unsigned tid;
     PRINT ("tunf: Set_replay_sleepset: set replay");
     _exec->set_replay (replay);

     if (opts::alt_algo == Altalgo::OPTIMAL) return;

     //  // otherwise we set sleeping the thread of every unjustified event in D that
     //  // is still enabled at J; this assumes that J contains C
     PRINT ("tunf: Set replay and sleep set: set SS ");
     _exec->clear_sleepset();
     for (auto e : d.unjustified)
     {
        ASSERT (e->action.type == ActionType::MTXLOCK);
        if (! j.ex_is_cex (e)) // J nay se xem lay o dau?
        {
           tid = replay.pidmap.get(e->pid());
           TRACE_ ("r%u (#%u) %p; ", tid, e->pid(), (void*) e->action.addr);
           _exec->add_sleepset (tid, (void*) e->action.addr);
        }
     }
     TRACE ("");
     PRINT ("tunf: set_replay_sleepset: Done");
}

// void Tunfolder:: _get_por_analysis ()
//{
//   PRINT ("tunf: get_por_analysis: inpath: %s", opts::inpath.c_str());
//   _load_bitcode (std::string (opts::inpath));
//
//   // set values for the argv and environ variables
//   _set_args (opts::argv);
//   _set_default_environment();
//}

} // namespace dpu





