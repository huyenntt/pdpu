
#ifndef _C15U_C15UNFOLDER_HH_
#define _C15U_C15UNFOLDER_HH_

#include "stid/executor.hh"

#include "pes/cut.hh"
#include "pes/event.hh"
#include "pes/config.hh"
#include "pes/process.hh"
#include "pes/unfolding.hh"

#include "unfolder/disset.hh"
#include "unfolder/trail.hh"
#include "unfolder/pidpool.hh"
#include "unfolder/replay.hh"
#include "unfolder/unfolder.hh"
//#include "unfolder/comb.hh"
#include "unfolder/alt-algorithm.hh"
#include "unfolder/task.hh"
#include "unfolder/tunfolder.hh"

#include "defectreport.hh"

#include <omp.h>
#include <queue>

namespace dpu
{

class C15unfolder : public Unfolder<C15unfolder>
{
public:
   /// counters to obtain statistics
   struct {
      bool timeout = false;
      long unsigned runs = 0;
      long unsigned ssbs = 0;
      long unsigned maxconfs = 0;
      long unsigned dupli = 0;
      long unsigned stid_threads = 0;
      float avg_max_trail_size = 0;
      struct {
         long unsigned calls = 0; // all calls
         long unsigned calls_built_comb = 0; // had to build the comb
         long unsigned calls_explore_comb = 0; // got to explore the comb
         Probdist<unsigned> spikes; // number of spikes
#ifdef CONFIG_STATS_DETAILED
         Probdist<unsigned> spikesizeunfilt; // size of the spikes before filtering out
         Probdist<unsigned> spikesizefilt; // size of the spikes after filtering out
#endif
      } alt;
   } counters;

   /// The list of all defects found during the exploration
   Defectreport report;

   /// Set to true for the explore() method to store each of the replays sent to
   /// Steroids in the field replays
   bool record_replays;

   /// All replay sequences sent to steroids will be recorded here if
   /// record_replays is true.
//   std::vector<stid::Replay> replays;
   std::vector<Replay> replays;

   /// Maximum number of seconds that the unfolder is able to work. Set to 0 to
   /// disable.
   unsigned timeout;

protected:
   /// Algorithm to compute alternatives
   Altalgo altalgo;

   /// When computing k-partial alternatives, the value of k
   unsigned kpartial_bound;


   omp_lock_t ulock; // unfolding lock
   omp_lock_t clock; // counters lock
   omp_lock_t rlock; // replays lock
   omp_lock_t slock; // start lock
   omp_lock_t pplock; // pidpool lock
   omp_lock_t rtlock; // report lock
   omp_nest_lock_t biglock; // biglock


//   std::vector<Task> tasks;


private:
   /// The comb data structure
//   Comb comb;

   /// Maximum number of context switches present in the trail for the
   /// exploration to allow computing alternatives for an event extracted from
   /// the trail immediately before.
   unsigned max_context_switches;

public:
   // Constructor
   C15unfolder (Altalgo a, unsigned kbound, unsigned maxcts);
   virtual ~C15unfolder ();

   /// Initialize and return the parameters of a stid::Executor
   stid::ExecutorConfig prepare_executor_config () const;

   std::unique_ptr<Tunfolder> _get_por_analysis ();

   /// runs the system up to completion using the replay, computes CEX of the
   /// resulting configuration, constructs a replay for each one of them and
   /// applies add_one_run for each one
   void add_multiple_runs (const Replay &r);

   // explores only one maximal configuration provided by tsk
//   void explore_one_maxconfig(Task *tsk, std::queue<Task> &tasks);
   void explore_one_maxconfig(Task *tsk);

   /// the CONCUR'15 POR algorithm
   void explore_para (); // without queue of tasks
//   void explore_para1 (); // with queue of tasks

   void explore_seq();
//   bool existed(Task *ntsk);
   bool rpl_existed(Replay rpl, std::vector<Replay> &rpl_list);

//   void explore_origin ();

//   /// compute the conflicting extensions of c and add them to a singly-linked
//   /// list pointed by head
   void compute_cex (Config &c, Event **head);
//
//   /// recursive function to explore all combinations in the comb of
//   /// alternatives
//   bool enumerate_combination (unsigned i, std::vector<Event*> &sol);
//
//   /// returns false only if no alternative to D \cup {e} after C exists
//   bool might_find_alternative (Config &c, Disset &d, Event *e);
//
//   /// finds one alternative for C after D, and stores it in J; we select from
//   /// here the specific algorithm that we call
//   bool find_alternative (const Trail &t, Config &c, const Disset &d, Cut &j);
//
//   /// Implementation 1: complete, unoptimal, searches conflict to only last event
//   bool find_alternative_only_last (const Config &c, const Disset &d, Cut &j);
//
//   /// Implementation 2: complete, optimal/unoptimal, based on the comb
//   bool find_alternative_kpartial (const Config &c, const Disset &d, Cut &j);
//
//   /// Implementation 2: complete, unoptimal
//   bool find_alternative_sdpor (Config &c, const Disset &d, Cut &j);
//
//   /// FIXME
//   void set_replay_and_sleepset (Replay &replay, const Cut &j, const Disset &d);
//
//   /// Computes conflicting extensions associated to event e
   void compute_cex_lock (Event *e, Event **head);

protected:

   inline void report_add_nondet_violation (const Trail &t, unsigned where,
         ActionType found);

   /// Translates the stream of actions into events, updating c, t, and d
   inline bool stream_to_events (Config &c, const stid::action_streamt &s,
         Trail *t = nullptr, Disset *d = nullptr, stid::Executor *exe = nullptr);

   /// Receives a stream, an iterator to that stream, a vector mapping stream
   /// pids to our pids, and a trail; this method advances the iterator of the
   /// stream asserting that the actions found match those of
   /// the trail; the iterator is left pointing at one plus the (blue) action
   /// matched with the last event in the trail; it also updates the pidmap at
   /// thread-creation events
   inline bool stream_match_trail (const stid::action_streamt &s,
         stid::action_stream_itt &it, Trail &t, Pidmap &pidmap);

   /// Initializes the fields of a Defectreport with the parameters of this
   /// C15unfolder.
   void report_init ();

   /// Returns debugging output suitable to be printed
   std::string explore_stat (const Trail &t, const Disset &d) const;
};

// implementation of inline methods
#include "unfolder/c15unfolder.hpp"

} //end of namespace
#endif

