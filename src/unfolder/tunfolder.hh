/*
 * tunfolder.hh
 *
 *  Created on: Apr 27, 2018
 *      Author: admin
 */

#ifndef SRC_UNFOLDER_TUNFOLDER_HH_
#define SRC_UNFOLDER_TUNFOLDER_HH_

#ifndef _UNFOLDER_HH_
#define _UNFOLDER_HH_

#include "stid/executor.hh"
#include "stid/action_stream.hh"

#include "pes/event.hh"
#include "pes/config.hh"
#include "pes/unfolding.hh"

#include "unfolder/replay.hh"
#include "unfolder/stream-converter.hh"

//#include "defectreport.hh" // FIXME: remove
//#include "redbox-factory.hh" // FIXME: remove

namespace dpu
{

class Tunfolder
{
public:

   /// A Steroids dynamic executor
   stid::Executor *_exec;

   // ctor and dtor
   Tunfolder (const stid::ExecutorConfig &config);
   Tunfolder (const Tunfolder &other);
   virtual ~Tunfolder ();

   /// Load the llvm module from the file \p filepath
   void _load_bitcode (std::string &&filepath);

   /// Saves the llvm module to a file
   void _store_bitcode (const std::string &filename) const;

   /// List all external symbols in the lodaded bytecode
   void _print_external_syms (const char *prefix);

   /// Sets the argv vector of the program to verify
   void _set_args (std::vector<const char *> argv);

   /// Sets the environment variables of the program to verify
   void _set_env (std::vector<const char *> env);

   /// Sets the environment variables of the program to verify to be a copy of
   /// our own environment, see environ(7)
   void _set_default_environment ();

   /// Runs the system up to completion (termination) using the provided replay
   /// and returns the corresponding maximal configuration. FIXME: the replay
   /// should be a dpu::Replay, but I temporarily reverted it to a stid::Replay
   /// to get something working ...
//   Config _add_one_run (const stid::Replay &r);

   /// determines if the causal closure of all events in eset is a configuration
   bool _is_conflict_free(const std::vector<Event *> &sol, const Event *e) const;

protected:

   /// A context object where LLVM will store the types and constants
   llvm::LLVMContext _context;

   /// File name for the llvm module under analysis
   std::string _path;

   /// LLVM module under analysis
   llvm::Module *_m;

   /// Configuration for the dynamic executor in Steroids
   stid::ExecutorConfig _config;
};

} //end of namespace

//// implementation of inline methods, outside of the namespace
//#include "unfolder/unfolder.hpp"

#endif

#endif /* SRC_UNFOLDER_TUNFOLDER_HH_ */
