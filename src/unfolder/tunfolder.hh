/*
 * tunfolder.hh
 *
 *  Created on: Apr 27, 2018
 *      Author: admin
 */

#ifndef SRC_UNFOLDER_TUNFOLDER_HH_
#define SRC_UNFOLDER_TUNFOLDER_HH_

#include "stid/executor.hh"
#include "stid/action_stream.hh"

#include "pes/event.hh"
#include "pes/config.hh"
#include "pes/unfolding.hh"

#include "unfolder/alt-algorithm.hh"
#include "unfolder/replay.hh"
#include "unfolder/stream-converter.hh"
#include "unfolder/disset.hh"
#include "unfolder/comb.hh"

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
   Tunfolder (const stid::ExecutorConfig &config, Altalgo a, unsigned kbound, unsigned maxcts); // Altalgo a, unsigned kbound, unsigned maxcts
//   Tunfolder (const Tunfolder &&other);
//   Tunfolder (const Tunfolder &other);
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
   bool _is_conflict_free (const std::vector<Event *> &sol, const Event *e) const;

   void _set_replay_sleepset (Replay &replay, const Disset &d, const Cut &j);

   /// recursive function to explore all combinations in the comb of
   /// alternatives
   bool enumerate_combination (unsigned i, std::vector<Event*> &sol);

   /// returns false only if no alternative to D \cup {e} after C exists
   bool might_find_alternative (Config &c, Disset &d, Event *e);

   /// finds one alternative for C after D, and stores it in J; we select from
   /// here the specific algorithm that we call
   bool find_alternative (const Trail &t, Config &c, const Disset &d, Cut &j, Unfolding &u);

   /// Implementation 1: complete, unoptimal, searches conflict to only last event
   bool find_alternative_only_last (const Config &c, const Disset &d, Cut &j);

   /// Implementation 2: complete, optimal/unoptimal, based on the comb
   bool find_alternative_kpartial (const Config &c, const Disset &d, Cut &j);

   /// Implementation 2: complete, unoptimal
   bool find_alternative_sdpor (Config &c, const Disset &d, Cut &j, Unfolding &u);

   /// Returns debugging output suitable to be printed
   std::string _explore_stat (const Trail &t, const Disset &d) const;

protected:

   /// A context object where LLVM will store the types and constants
   llvm::LLVMContext _context;

   /// File name for the llvm module under analysis
   std::string _path;

   /// LLVM module under analysis
   llvm::Module *_m;

   /// Configuration for the dynamic executor in Steroids
   stid::ExecutorConfig _config;

   /// Algorithm to compute alternatives
   Altalgo alt_algo;

   /// When computing k-partial alternatives, the value of k
   unsigned kpartial_bound;

private:
   /// The comb data structure
   Comb comb;

   /// Maximum number of context switches present in the trail for the
   /// exploration to allow computing alternatives for an event extracted from
   /// the trail immediately before.
//   unsigned max_context_switches; // De xem cho nay the nao -> FIXME
};

} //end of namespace

#endif /* SRC_UNFOLDER_TUNFOLDER_HH_ */
