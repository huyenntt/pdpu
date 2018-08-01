
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
#include <vector>
#include <string>
#include <queue>

//#include "stid/action_stream.hh"
#include "stid/executor.hh"

#include "unfolder/c15unfolder.hh" // must be before verbosity.h
#include "misc.hh"
#include "verbosity.h"
#include "pes/process.hh"
#include "opts.hh"

namespace dpu
{

C15unfolder::C15unfolder (Altalgo a, unsigned kbound, unsigned maxcts) :
   Unfolder (prepare_executor_config ()),
   report (),
   record_replays (true), // Huyen changed to true
   replays (),
   timeout (0),
   altalgo (a),
   kpartial_bound (kbound),
//   comb (a, kbound),
   max_context_switches (maxcts)
{
   std::string s;

   if (altalgo == Altalgo::OPTIMAL)
      kpartial_bound = UINT_MAX;

   if (altalgo != Altalgo::OPTIMAL and maxcts != UINT_MAX)
   {
      s = "Limiting the number of context switches is only possible " \
         "with optimal alternatives";
      throw std::invalid_argument (s);
   }

   // Initialize the lock for the unfolding
//   proc_locks.reserve(MAX_PROC);
   omp_init_lock(&ulock);
   omp_init_lock(&clock);
   omp_init_lock(&rlock);
   omp_init_lock(&slock);
   omp_init_lock(&pplock);

}

C15unfolder::~C15unfolder ()
{
   DEBUG ("c15u.dtor: this %p", this);

   // Destroy the lock of the unfolding
   omp_destroy_lock(&ulock);
   omp_destroy_lock(&clock);
   omp_destroy_lock(&rlock);
   omp_destroy_lock(&slock);
   omp_destroy_lock(&pplock);
}

stid::ExecutorConfig C15unfolder::prepare_executor_config () const
{
   stid::ExecutorConfig conf;

   conf.memsize = opts::memsize;
   conf.defaultstacksize = opts::stacksize;
   conf.optlevel = opts::optlevel;
   conf.tracesize = CONFIG_GUEST_TRACE_BUFFER_SIZE;

   conf.flags.dosleep = opts::dosleep ? 1 : 0;
   conf.flags.verbose = opts::verbosity >= 3 ? 1 : 0;

   unsigned i = opts::strace ? 1 : 0;
   conf.strace.fs = i;
   conf.strace.pthreads = i;
   conf.strace.malloc = i;
   conf.strace.proc = i;
   conf.strace.others = i;

   conf.do_load_store = false;
   return conf;
}

void C15unfolder::add_multiple_runs (const Replay &r)
{
   Event *e;
   Replay rep (u);
   std::vector<Event *>cex;

   Config c (add_one_run (r));
   // add_one_run executes the system up to completion, we now compute all cex
   // of the resulting configuration and iterate through them
   c.dump ();

   // compute cex
   e = nullptr;
   compute_cex (c, &e);

   // copy the cex into a vector
   for (; e; e = e->next) cex.push_back (e);

   // run the system once more for every cex
   for (Event *e : cex)
   {
      rep.clear ();
      rep.extend_from (e->cone);
      rep.push_back ({-1, -1});
      add_one_run (rep);
   }
}

std::string C15unfolder::explore_stat (const Trail &t, const Disset &d) const
{
   unsigned u, j;

   u = j = 0;
   for (auto it = d.justified.begin(), end = d.justified.end();
         it != end; ++it) j++;
   for (auto it = d.unjustified.begin(), end = d.unjustified.end();
         it != end; ++it) u++;

   // 37e  9j  8u
   return fmt ("%2ue %2uj %2uu", t.size(), j, u);
}

std::unique_ptr<Tunfolder> C15unfolder:: _get_por_analysis () // Lay cac tham so truc tiep tu C15, ko can phai truyen vao ham
{
     std::unique_ptr<Tunfolder> tunf;

     // build a new tunfolder
     tunf.reset (new Tunfolder (prepare_executor_config (), altalgo, kpartial_bound, max_context_switches)); // O day moi set up config va cac tham so khac
     tunf->_load_bitcode (std::string (opts::inpath));

     // set values for the argv and environ variables
     tunf->_set_args (opts::argv);
     tunf->_set_default_environment();

//     // configure the timeout
//     tunf->timeout = opts::timeout;

     // Initialize start in C15 for new maximal configuration in stream_to_events - Xem xet lai sau

     return tunf;
}
//
//void C15unfolder::explore_one_maxconfig (Task *tsk, std::queue<Task> &tasks)
//{
//      int i = 0;
//      Event *e = nullptr;
//      bool b;
//      Replay replay(u);
//      Cut j (Unfolding::MAX_PROC);
//      std::unique_ptr<Tunfolder> unfolder;
//      time_t start;
//      start = time (nullptr);
////      Task *ntsk;
//
//      PRINT ("c15::explore: call get_por_analysis for tunfolder");
//      unfolder = _get_por_analysis();
//      replay.build_from (tsk->trail, tsk->conf, tsk->add);
////      PRINT ("replay: %s", replay.str().c_str());
//
//      unfolder->_set_replay_sleepset(replay, tsk->dis, tsk->add); // Cho nay chua the hien add
//      PRINT ("c15: explore: call run from steroids");
//      unfolder->_exec->run();
//
//      // if requested, record the replay sequence
//      if (record_replays) replays.push_back (replay);
//
//      // Get a trace from stream
//      stid::action_streamt s (unfolder->_exec->get_trace ());
//
//        counters.runs++;
//        i = s.get_rt()->trace.num_ths;
//        if (counters.stid_threads < i) counters.stid_threads = i;
//
//        s.print ();
//  //      tsk->trail.dump();
//        /* đến đây vẫn chưa dùng gì đến flags.ind của các events trong D*/
//        PRINT ("c15u: explore: Stream to events:");
//        /*
//         * We need to store old trail here, or just top_idx of trail to avoid backtracking the events which are
//         * considered for alternatives before.
//         */
//        Event * last_old_trail = tsk->trail.empty() ? nullptr : tsk->trail.peek();
//        int last_trail_size = tsk->trail.size();
//
//  //      if (last_old_trail)
//  //         PRINT ("last_old_trail: %s", last_old_trail->str().c_str());
//  //      else
//  //         PRINT ("empty trail-> last evt is bottom");
////        PRINT ("c15: explore: old trail size: %d trail size: %zu", last_trail_size, tsk->trail.size());
//
//        // Thuc ra stream to events cung chua can dung den flags.ind
//        // Ham nay lam viec chu yeu voi unfolding -> Can lock
//         b = stream_to_events (tsk->conf, s, &tsk->trail, &tsk->dis, unfolder->_exec); // Phai xu ly voi d,c của task-> DONE!
//
//         // b could be false because of SSBs or defects - TAM BO HIEN THI THONG TIN CHO DE THEO DOI CAC THONG TIN KHAC
//  //        PRINT ("c15u: explore: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
//          #ifdef VERB_LEVEL_TRACE
//             if (verb_trace)
//                tsk->trail.dump2 (fmt ("c15u: explore: %s: ", explore_stat(tsk->trail,tsk->dis).c_str()).c_str());
//  //           if (verb_debug) tsk->conf.dump ();
//  //           if (verb_debug) tsk->dis.dump ();
//          #endif
//
//          // add conflicting extensions
//          PRINT ("c15: explore: compute cex");
//          compute_cex (tsk->conf, &e);  // Truy cap den unfolding cuar C15unfolder, lock se dung ben trong ham
//
//          counters.avg_max_trail_size += tsk->trail.size(); //???
//
//          // backtrack until we find some right subtree to explore
//  //        DEBUG ("");
//          PRINT("c15: explore: backtrach the trail============================");
//
////          if (last_old_trail)
////             PRINT ("c15: explore: last-event_trail: %s", last_old_trail->str().c_str() );
//
//          tsk->dis.set_flags();
//  //        PRINT ("c15: explore: after set flags: ");
//  //        tsk->dis.dump();
//          while (tsk->trail.size() > last_trail_size) // Ko xet lai event da tim thay alternative o luc truoc, last event in old trail
//          {
//             e = tsk->trail.pop ();
//                // pop last event out of the trail/config; indicate so to the disset
//             PRINT ("c15u: explore: %s: popping: i %2zu ncs %u %s",
//                       explore_stat(tsk->trail,tsk->dis).c_str(), tsk->trail.size(), tsk->trail.nr_context_switches(),
//                       e->str().c_str());
//
//             tsk->conf.unfire (e);
//
//  //           PRINT ("Make sure that all events in disset are set flags.ind");
//             tsk->dis.trail_pop (tsk->trail.size ()); // Hàm này dùng flags.ind!!! Ma ham nay de lam gi quen roi
//  //           tsk->dis.unset_flags();
//             // skip searching for alternatives if we exceed the number of allowed
//             // context switches
//             if (tsk->trail.nr_context_switches() >= max_context_switches)
//                PRINT ("c15u: explore: %s: continue", explore_stat(tsk->trail,tsk->dis).c_str());
//             if (tsk->trail.nr_context_switches() >= max_context_switches) continue;
//
//             // check for alternatives
//             counters.alt.calls++;
//             if (! unfolder->might_find_alternative (tsk->conf, tsk->dis, e))
//             {
//                PRINT ("c15: epxplore: no possiblility to get an alternative");
//                   continue;
//             }
//
//  //           PRINT ("c15: explore: trail.size %zu", tsk->trail.size());
//             tsk->dis.add (e, tsk->trail.size()); // Phan khoi tao tsk->dis co van de -> Done with cctor!
//
//             // Doi j thanh tsk->add vi khi 1 alternative duoc tim thay, no se duoc luu trong j.
//  //           Dung luon tsk->add de ko phai copy nua?? Ah khong, dang nao thi khi tao task moi cung can phai copy j
//             /* Van de la dung cac member cua task co tien hon cho viec parallel sau nay ko?*/
//             if (unfolder->find_alternative (tsk->trail, tsk->conf, tsk->dis, tsk->add, u))
//             {
//                   // Here we create a new task to explore new branch with the alternative found
//                   PRINT ("c15: explore: an alternative found");
////                   replay.build_from (tsk->trail, tsk->conf, tsk->add);
//                   // Phai set sleep set va replay moi cho executor o day
//  //                 tsk->dis.dump();
//                   tasks.emplace (tsk->dis, tsk->add, tsk->trail, tsk->conf);
//  //                 tasks.emplace (replay, tsk->dis, tsk->add, tsk->trail, tsk->conf);
//                   PRINT ("c15: explore: new task inserted in tasks");
//                   tasks.back().dump(); // Den cho nay ind = -1
//              } // end of if
//             else
//                PRINT ("c15: epxplore: no alt found");
//
//             tsk->dis.unadd (); // Chi co duy nhat 1 event cuoi cung tro ve 0.
//
//             // Break the loop if backtracking meets an explored event
//  //           if (tsk->trail.peek() == last_old_trail) break;
//
////             if (counters.runs % 10 == 0 and timeout)
////                if (time(nullptr) - start_time > timeout)
////                {
////                    counters.timeout = true;
////                    break;
////                }
//
//             counters.ssbs += tsk->dis.ssb_count;
//          } // end of while trail
//
//         tsk->dis.unset_flags();
//         PRINT ("c15: explore: stop backtracking==========================");
//          // if we exhausted the time cap, we stop
//     // statistics (all for c15unfolder) - Minh can xem lai cho tong ket thong tin 1 chut
//}

bool C15unfolder:: existed (Task *ntsk, std::vector<Task> &full_tasks)
{
   for (auto &t: full_tasks)
      if (*ntsk == t)
         return true;

   return false;
}
//================
bool C15unfolder:: rpl_existed (Replay rpl, std::vector<Replay> &rpl_list)
{
   for (auto &r : rpl_list)
//      if ( (rpl == r) or rpl.is_derived(r))
   if (rpl == r)
         return true;

   return false;
}

//===================================
void C15unfolder::explore_one_maxconfig (Task *tsk)
{
   int i = 0;
   Event *e = nullptr;
   bool b;
   Replay replay(u);
   Cut j (Unfolding::MAX_PROC);
   std::unique_ptr<Tunfolder> unfolder;
   time_t start;
   start = time (nullptr);
   Task *ntsk;

   PRINT ("c15u::explore: call get_por_analysis for tunfolder");
   unfolder = _get_por_analysis();
//   replay.build_from (tsk->trail, tsk->conf, tsk->add);
//   PRINT ("replay: %s", replay.str().c_str());
//   PRINT ("replay: %s", tsk->rep.str().c_str());

//   unfolder->_set_replay_sleepset(replay, tsk->dis, tsk->add);
   unfolder->_set_replay_sleepset(tsk->rep, tsk->dis, tsk->add);
   PRINT ("c15u: explore: call run from steroids");
   unfolder->_exec->run();

   // Get a trace from stream
   stid::action_streamt s (unfolder->_exec->get_trace ());

   omp_set_lock(&clock);
     // if requested, record the replay sequence
//     if (record_replays) replays.push_back (replay);
//   if (record_replays) replays.push_back (tsk->rep); // always push a new replay to list of replays
     counters.runs++;
     i = s.get_rt()->trace.num_ths;
     if (counters.stid_threads < i)
        counters.stid_threads = i;
   omp_unset_lock(&clock);

//     s.print ();
//      tsk->trail.dump();

     PRINT ("c15u: explore: Stream to events:");
     /*
      * We need to store old trail here, or just top_idx of trail to avoid backtracking the events which are
      * considered for alternatives before.
      */
//     Event * last_old_trail = tsk->trail.empty() ? nullptr : tsk->trail.peek();
     int last_trail_size = tsk->trail.size();

      omp_set_lock(&clock);
         b = stream_to_events (tsk->conf, s, &tsk->trail, &tsk->dis, unfolder->_exec); // Phai xu ly voi d,c của task-> DONE!
      omp_unset_lock(&clock);

      // Chi tang run khi nao stream_to_events phat sinh event moi, ko thi thoi

      // b could be false because of SSBs or defects - TAM BO HIEN THI THONG TIN CHO DE THEO DOI CAC THONG TIN KHAC
//        PRINT ("c15u: explore: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
       #ifdef VERB_LEVEL_TRACE
          if (verb_trace)
             tsk->trail.dump2 (fmt ("c15u: explore: %s: ", explore_stat(tsk->trail,tsk->dis).c_str()).c_str());
//           if (verb_debug) tsk->conf.dump ();
//           if (verb_debug) tsk->dis.dump ();
       #endif

       // add conflicting extensions
       PRINT ("c15u: explore: compute cex");
          compute_cex (tsk->conf, &e);  // Truy cap den unfolding cuar C15unfolder, lock se dung ben trong ham

       omp_set_lock(&clock);
          counters.avg_max_trail_size += tsk->trail.size();
       omp_unset_lock(&clock);

       // backtrack until we find some right subtree to explore
       PRINT("c15u: explore: backtrach the trail");

//       while (tsk->trail.size() > last_trail_size) // Ko xet lai event da tim thay alternative o luc truoc, last event in old trail
       while (tsk->trail.size() > 0)
       {
          e = tsk->trail.pop ();
          // pop last event out of the trail/config; indicate so to the disset
          PRINT ("c15u: explore: %s: popping: i %2zu ncs %u %s",
                    explore_stat(tsk->trail,tsk->dis).c_str(), tsk->trail.size(), tsk->trail.nr_context_switches(),
                    e->str().c_str());

          tsk->conf.unfire (e);
          tsk->dis.trail_pop (tsk->trail.size ());

          // skip searching for alternatives if we exceed the number of allowed
          // context switches
          if (tsk->trail.nr_context_switches() >= max_context_switches)
             PRINT ("c15u: explore: %s: continue", explore_stat(tsk->trail,tsk->dis).c_str());
          if (tsk->trail.nr_context_switches() >= max_context_switches) continue;

          // check for alternatives
          omp_set_lock(&clock);
             counters.alt.calls++;
          omp_unset_lock(&clock);

          if (! unfolder->might_find_alternative (tsk->conf, tsk->dis, e))
          {
//             PRINT ("c15: epxplore: no possiblility to get an alternative");
                continue;
          }

//           PRINT ("c15: explore: trail.size %zu", tsk->trail.size());
          tsk->dis.add (e, tsk->trail.size());

          // Doi j thanh tsk->add vi khi 1 alternative duoc tim thay, no se duoc luu trong j.
//           Dung luon tsk->add de ko phai copy nua?? Ah khong, dang nao thi khi tao task moi cung can phai copy j
          if (unfolder->find_alternative (tsk->trail, tsk->conf, tsk->dis, tsk->add, u))
          {
                // Here we create a new task to explore new branch with the alternative found
                PRINT ("c15u: explore: an alternative found");
                replay.build_from (tsk->trail, tsk->conf, tsk->add);
                if (tsk->trail.size() <= last_trail_size)
                {
                   if (rpl_existed (replay,replays))
                   {
//                       PRINT ("c15u: explore: task already exists");
                       continue;
                   }
                } // end of if trail size

                omp_set_lock(&rlock);
                   replays.push_back(replay);
                omp_unset_lock(&rlock);

                ntsk = new Task(replay, tsk->dis, tsk->add, tsk->trail, tsk->conf);
//                ntsk = new Task(tsk->dis, tsk->add, tsk->trail, tsk->conf);
                // Can phai push task vafo full_tasks o day
                #pragma omp task firstprivate(ntsk)
                {
                   explore_one_maxconfig(ntsk);
                }
           } // end of if
//          else
//             PRINT ("c15u: epxplore: no alt found");

          tsk->dis.unadd ();

       } // end of while trail

       omp_set_lock(&clock);
          counters.ssbs += tsk->dis.ssb_count;
       omp_unset_lock(&clock);

      PRINT ("c15u: explore: stop backtracking==========================");
}
//===========================epxplore==================
void C15unfolder::explore_para ()
{
   Trail t;
   Disset d;
   Config c (Unfolding::MAX_PROC);
   Cut j (Unfolding::MAX_PROC);
   Replay replay (u);
//   std::queue<Task> tasks;
//   Task *tsk = new Task(d,j,t,c);
   Task *tsk = new Task(replay,d,j,t,c);
   time_t start_time;

   //   PRINT ("c15: explore: Replay at the beginning: %s", replay.str().c_str());
   // initialize the defect report now that all settings for this verification
   // exploration are fixed
   report_init (); // init report trong c15
   start_time = time (nullptr);

//   omp_set_num_threads(5);
//   PRINT ("CORESSSS %u", opts::cores);
   omp_set_num_threads(opts::cores);

   #pragma omp parallel firstprivate(tsk)
   {
      #pragma omp single
      {
//         PRINT ("c15: explore: task outside the omp task");
//         tsk->dump();
//         #pragma omp task firstprivate(tsk)
//         {
            explore_one_maxconfig(tsk);
//         }
      } // end of single
   } // end of parallel

//   #pragma omp taskwait

   // statistics (all for c15unfolder)
//   counters.ssbs = tsk->dis.ssb_count;
   counters.maxconfs = counters.runs - counters.ssbs - counters.dupli; // Buon cuoi that, tai sao lai cu lon hon so thuc 1 lan nhi???
   counters.avg_max_trail_size /= counters.runs;
   PRINT ("c15u: explore: done!");
   ASSERT (counters.ssbs == 0 or altalgo != Altalgo::OPTIMAL);
}
//===========================explore sequence=================================
void C15unfolder:: explore_seq()
{
   bool b;
   Trail t;
   Disset d;
   Config c (Unfolding::MAX_PROC);
   Cut j (Unfolding::MAX_PROC);
   Replay replay (u);
   Event *e = nullptr;
   int i = 0;
   time_t start_time;
   std::unique_ptr<Tunfolder> unfolder;

   Task *tsk;
   std::queue<Task> tasks;

   std::vector<Task> full_tasks;
   std::vector<Replay> rpl_list;

//   Task *ntsk;

   start_time = time (nullptr);

//   PRINT ("c15: explore: Replay at the beginning: %s", replay.str().c_str());
   // initialize the defect report now that all settings for this verification
   // exploration are fixed
   report_init (); // init report trong c15

   int tcount = 1;
   PRINT ("c15: explore: initialize the queue");
//   tasks.emplace(d, j, t, c); // first task with all empty, // J inlucdes C, so C is unnecessary
   tasks.emplace(replay, d, j, t, c);
   full_tasks.emplace_back(replay, d, j, t, c);
//   rpl_list.push_back(replay); // forget the first replay

   while (!tasks.empty())
   {
      PRINT ("c15: explore: POP A NEW TASK FROM QUEUE");
      ASSERT (!tasks.empty());
      tsk = new Task(std::move(tasks.front()));
      tasks.pop();
      tsk->dump();

      PRINT ("c15::explore:: tasks.size: %lu", tasks.size());

      PRINT ("c15::explore: call get_por_analysis for tunfolder");
      unfolder = _get_por_analysis();
//      replay.build_from (tsk->trail, tsk->conf, tsk->add);
//      PRINT ("replay: %s", replay.str().c_str());

      unfolder->_set_replay_sleepset(tsk->rep, tsk->dis, tsk->add); // Cho nay chua the hien add

      PRINT ("c15: explore: call run from steroids");
      unfolder->_exec->run();

      // if requested, record the replay sequence
      if (record_replays) replays.push_back (replay);

      // Get a trace from stream
      stid::action_streamt s (unfolder->_exec->get_trace ());

      counters.runs++;
      i = s.get_rt()->trace.num_ths;
      if (counters.stid_threads < i) counters.stid_threads = i;

//      s.print ();
//      tsk->trail.dump();
      PRINT ("c15u: explore: Stream to events:");

      /*
       * We need to store old trail here, or just top_idx of trail to avoid backtracking the events which are
       * considered for alternatives before.
       */
//      Event * last_old_trail = tsk->trail.empty() ? nullptr : tsk->trail.peek();
      int last_trail_size = tsk->trail.size();

//      if (last_old_trail)
//         PRINT ("last_old_trail: %s", last_old_trail->str().c_str());
//      else
//         PRINT ("empty trail-> last evt is bottom");

       b = stream_to_events (tsk->conf, s, &tsk->trail, &tsk->dis, unfolder->_exec);

       // b could be false because of SSBs or defects
//        PRINT ("c15u: explore: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
//        #ifdef VERB_LEVEL_TRACE
//           if (verb_trace)
//              tsk->trail.dump2 (fmt ("c15u: explore: %s: ", explore_stat(tsk->trail,tsk->dis).c_str()).c_str());
//           if (verb_debug) tsk->conf.dump ();
//           if (verb_debug) tsk->dis.dump ();
//        #endif

        // add conflicting extensions
        PRINT ("c15: explore: compute cex");
        compute_cex (tsk->conf, &e);  // Truy cap den unfolding cuar C15unfolder, lock se dung ben trong ham

        counters.avg_max_trail_size += tsk->trail.size(); //???

        // backtrack until we find some right subtree to explore
//        DEBUG ("");
        PRINT("c15: explore: backtrach the trail============================");

        PRINT ("c15: explore: old trail size: %d trail size: %zu", last_trail_size, tsk->trail.size());

        tsk->dis.dump();

//        while (tsk->trail.size() > last_trail_size) // Ko xet lai event da tim thay alternative o luc truoc, last event in old trail
        while (tsk->trail.size() > 0)
        {
           e = tsk->trail.pop ();
              // pop last event out of the trail/config; indicate so to the disset
           PRINT ("c15u: explore: %s: popping: i %2zu ncs %u %s",
                     explore_stat(tsk->trail,tsk->dis).c_str(), tsk->trail.size(), tsk->trail.nr_context_switches(),
                     e->str().c_str());

           tsk->conf.unfire (e);

           tsk->dis.trail_pop (tsk->trail.size ()); // Hàm này dùng flags.ind!!! Ma ham nay de lam gi quen roi

           // skip searching for alternatives if we exceed the number of allowed
           // context switches
           if (tsk->trail.nr_context_switches() >= max_context_switches)
              PRINT ("c15u: explore: %s: continue", explore_stat(tsk->trail,tsk->dis).c_str());
           if (tsk->trail.nr_context_switches() >= max_context_switches) continue;

           // check for alternatives
           counters.alt.calls++;
           if (! unfolder->might_find_alternative (tsk->conf, tsk->dis, e))
           {
              PRINT ("c15u: epxplore: no possiblility to get an alternative");
                 continue;
           }

//           PRINT ("c15: explore: trail.size %zu", tsk->trail.size());
           tsk->dis.add (e, tsk->trail.size());

           if (unfolder->find_alternative (tsk->trail, tsk->conf, tsk->dis, tsk->add, u))
           {
                 // Here we create a new task to explore new branch with the alternative found
                 PRINT ("c15u: explore: an alternative found");
                 replay.build_from (tsk->trail, tsk->conf, tsk->add);
//                    if (existed(ntsk,full_tasks))
                 if (rpl_existed (replay,rpl_list))
                 {
                    PRINT ("c15u: explore: task already exists");
                    continue;
                 }
//                 ntsk = new Task (replay, tsk->dis, tsk->add, tsk->trail, tsk->conf);
//                 tasks.push(std::move(*ntsk));
//                 PRINT("New task after tasks.push");
//                 ntsk->dump(); // move chua gan ntsk ve null het
//                 full_tasks.push_back(std::move(*ntsk));
//                 full_tasks.emplace_back(replay, tsk->dis, tsk->add, tsk->trail, tsk->conf);
//                 tasks.emplace (tsk->dis, tsk->add, tsk->trail, tsk->conf);

                 tasks.emplace (replay, tsk->dis, tsk->add, tsk->trail, tsk->conf);
                 rpl_list.push_back(replay);
                 tcount++;
                 PRINT ("c15u: explore: new task inserted in tasks");
//                 tasks.back().dump();
            } // end of if
           else
              PRINT ("c15u: epxplore: no alt found");

           tsk->dis.unadd (); // Chi co duy nhat 1 event cuoi cung tro ve 0.

           if (counters.runs % 10 == 0 and timeout)
              if (time(nullptr) - start_time > timeout)
              {
                  counters.timeout = true;
                  break;
              }
        } // end of while trail

       PRINT ("c15u: explore: stop backtracking==========================");

       counters.ssbs += tsk->dis.ssb_count;
   } // End of while tasks
   // statistics (all for c15unfolder) - Minh can xem lai cho tong ket thong tin 1 chut

   PRINT ("c15u: all the replays");
   for (int i = 0; i < rpl_list.size(); i++)
      PRINT ("%s", rpl_list[i].str().c_str());

   counters.maxconfs = counters.runs - counters.ssbs - counters.dupli;
   counters.avg_max_trail_size /= counters.runs;
   PRINT ("c15u: explore: done!, number of tasks: %d",tcount);
   ASSERT (counters.ssbs == 0 or altalgo != Altalgo::OPTIMAL); // Xem lai cho nay
}
//void C15unfolder::explore_origin ()
//{
//   bool b;
//   Trail t;
//   Disset d;
//   Config c (Unfolding::MAX_PROC);
//   Cut j (Unfolding::MAX_PROC);
//   Replay replay (u);
//   Event *e = nullptr;
//   int i = 0;
//   time_t start;
//
//   // initialize the defect report now that all settings for this verification
//   // exploration are fixed
//   report_init ();
//   start = time (nullptr);
//
//   int a = 1;
//   while (a)
//   {
//      // if requested, record the replay sequence
//      if (record_replays) replays.push_back (replay);
//
//      // explore the leftmost branch starting from our current node
//      DEBUG ("c15u: explore: %s: running the system...",
//            explore_stat (t, d).c_str());
//      exec->run ();
//      stid::action_streamt s (exec->get_trace ());
//      counters.runs++;
//      i = s.get_rt()->trace.num_ths;
//      if (counters.stid_threads < i) counters.stid_threads = i;
//      DEBUG ("c15u: explore: the stream: %s:", explore_stat (t,d).c_str());
//#ifdef VERB_LEVEL_DEBUG
//      if (verb_debug) s.print ();
//#endif
//      b = stream_to_events (c, s, &t, &d);
//      // b could be false because of SSBs or defects
//      DEBUG ("c15u: explore: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
//#ifdef VERB_LEVEL_TRACE
//      if (verb_trace)
//         t.dump2 (fmt ("c15u: explore: %s: ", explore_stat(t,d).c_str()).c_str());
//      if (verb_debug) pidpool.dump ();
//      if (verb_debug) c.dump ();
//      if (verb_debug) d.dump ();
//#endif
//
//      // add conflicting extensions
//      compute_cex (c, &e);
//      counters.avg_max_trail_size += t.size();
//
//      // backtrack until we find some right subtree to explore
//      DEBUG ("");
//      while (t.size())
//      {
//         // pop last event out of the trail/config; indicate so to the disset
//         e = t.pop ();
//         PRINT ("c15u: explore: %s: popping: i %2zu ncs %u %s",
//               explore_stat(t,d).c_str(), t.size(), t.nr_context_switches(),
//               e->str().c_str());
//         c.unfire (e);
//         d.trail_pop (t.size ());
//
//         // skip searching for alternatives if we exceed the number of allowed
//         // context switches
//         if (t.nr_context_switches() >= max_context_switches)
//            PRINT ("c15u: explore: %s: continue", explore_stat(t,d).c_str());
//         if (t.nr_context_switches() >= max_context_switches) continue;
//
//         // check for alternatives
//         counters.alt.calls++;
//         if (! might_find_alternative (c, d, e)) continue;
//         d.add (e, t.size());
//         if (find_alternative (t, c, d, j)) break;
//         d.unadd ();
//      }
//
//      // if we exhausted the time cap, we stop
//      if (counters.runs % 10 == 0 and timeout)
//         if (time(nullptr) - start > timeout)
//            { counters.timeout = true; break; }
//
//      // if the trail is now empty, we finished; otherwise we compute a replay
//      // and pass it to steroids
//      if (! t.size ()) break;
//      replay.build_from (t, c, j);
//      set_replay_and_sleepset (replay, j, d);
//
//      // explore only one execution
////      a = 0;
//   }
//
//   // statistics
//   counters.ssbs = d.ssb_count;
//   counters.maxconfs = counters.runs - counters.ssbs;
//   counters.avg_max_trail_size /= counters.runs;
//   DEBUG ("c15u: explore: done!");
//   ASSERT (counters.ssbs == 0 or altalgo != Altalgo::OPTIMAL);
//}
//

//void C15unfolder::set_replay_and_sleepset (Replay &replay, const Cut &j,
//      const Disset &d)
//{
//   unsigned tid;
//
//   exec->set_replay (replay);
//
//   // no need for sleepsets if we are optimal
//   if (altalgo == Altalgo::OPTIMAL) return;
//
//   // otherwise we set sleeping the thread of every unjustified event in D that
//   // is still enabled at J; this assumes that J contains C
//   TRACE_ ("c15u: explore: sleep set: ");
//   exec->clear_sleepset();
//   for (auto e : d.unjustified)
//   {
//      ASSERT (e->action.type == ActionType::MTXLOCK);
//      if (! j.ex_is_cex (e))
//      {
//         PRINT ("Chet o day a?");
//         tid = replay.pidmap.get(e->pid());
//         TRACE_ ("r%u (#%u) %p; ", tid, e->pid(), (void*) e->action.addr);
//         exec->add_sleepset (tid, (void*) e->action.addr);
//      }
//   }
//   TRACE ("");
//}

void C15unfolder::compute_cex_lock (Event *e, Event **head)
{
   // 1. let ep be the pre-proc of e
   // 2. let em be the pre-mem of e
   //
   // 3. if em <= ep, then return   // there is no cex
   // 4. em = em.pre-em             // em is now a LOCK
   // 5. if (em <= ep) return       // because when you fire [ep] the lock is acquired!
   // 6. em = em.pre-mem            // em is now an UNLOCK
   // 7. insert event (e.action, ep, em)
   // 8. goto 3

   Event *ep, *em, *ee;

//   PRINT ("c15u: cex-lock: starting from %s", e->str().c_str());
   ASSERT (e)
   ASSERT (e->action.type == ActionType::MTXLOCK);

   // 1,2: ep/em are the predecessors in process/memory
   ep = e->pre_proc();
   em = e->pre_other();

   while (1)
   {
      // 3. we are done if we got em <= ep (em == null means em = bottom!)
      if (!em or em->is_predeq_of (ep)) return;

      // 4,5: jump back 1 predecessor in memory, if we got em <= ep, return
      ASSERT (em->action.type == ActionType::MTXUNLK);
      em = em->pre_other();
      ASSERT (em);
      ASSERT (em->action.type == ActionType::MTXLOCK);
      if (em->is_predeq_of (ep)) return;

      // 6. back 1 more predecessor
      em = em->pre_other();
      ASSERT (!em or em->action.type == ActionType::MTXUNLK);

      // 7. (action, ep, em) is a possibly new event
//      omp_set_lock(&ulock);
      omp_set_lock(&ep->process()->plock);
         ee = u.event (e->action, ep, em);
      omp_unset_lock(&ep->process()->plock);
//      omp_unset_lock(&ulock);

//      PRINT ("c15u: cex-lock:  new cex: %s", ee->str().c_str());

      // we add it to the linked-list
      ee->next = *head; // next is also used in cut_to_replay
      *head = ee;
   }
}

void C15unfolder::compute_cex (Config &c, Event **head)
{
   Event *e;

   PRINT ("c15u: cex: c %p *head %p |mm| %lu", &c, *head, c.mutexmax.size());

   for (auto const & max : c.mutexmax)
   {
      // skip events that are not locks or unlocks
      e = max.second;
//      PRINT ("Event: %s", e->str().c_str());
      ASSERT (e);
      if (e->action.type != ActionType::MTXUNLK and
            e->action.type != ActionType::MTXLOCK) continue;

      // scan the lock chain backwards
      for (; e; e = e->pre_other())
      {
         if (e->action.type == ActionType::MTXLOCK)
         {
            compute_cex_lock (e, head);
         }
      }
   }
}
//
//bool C15unfolder::enumerate_combination (unsigned i,
//   std::vector<Event*> &sol)
//{
//   // We enumerate combinations using one event from each spike
//   // - the partial solution is stored in sol
//   // - if sol is conflict-free, we return true
//
//   ASSERT (i < comb.size());
//   for (auto e : comb[i])
//   {
//      if (! is_conflict_free (sol, e)) continue;
//      sol.push_back (e);
//      if (sol.size() == comb.size()) return true;
//      if (enumerate_combination (i+1, sol)) return true;
//      sol.pop_back ();
//   }
//   return false;
//}
//
//bool C15unfolder::might_find_alternative (Config &c, Disset &d, Event *e)
//{
//   // this method can return return false only if we are totally sure that no
//   // alternative to D \cup e exists after C; this method should run fast, it
//   // will be called after popping every single event event from the trail; our
//   // implementation is very fast: we return false if the event type is != lock
//
//   // e->icfls() is nonempty => e is a lock
//   ASSERT (! e->icfl_count() or e->action.type == ActionType::MTXLOCK);
//
//   return e->action.type == ActionType::MTXLOCK;
//}
//
//inline bool C15unfolder::find_alternative (const Trail &t, Config &c, const Disset &d, Cut &j)
//{
//   bool b;
//
//   switch (altalgo) {
//   case Altalgo::OPTIMAL :
//   case Altalgo::KPARTIAL :
//      b = find_alternative_kpartial (c, d, j);
//      break;
//   case Altalgo::ONLYLAST :
//      b = find_alternative_only_last (c, d, j);
//      break;
//   case Altalgo::SDPOR :
//      b = find_alternative_sdpor (c, d, j);
//      break;
//   }
//
//   // no alternative may intersect with d
//   if (b)
//   {
//      //if (d.intersects_with (j)) { d.dump (); j.dump (); }
//      ASSERT (! d.intersects_with (j));
//   }
//
//   TRACE_ ("c15u: explore: %s: alt: [", explore_stat (t, d).c_str());
//#ifdef VERB_LEVEL_TRACE
//   if (verb_trace)
//   {
//      std::vector<Event*> v;
//      for (auto e : d.unjustified)
//      {
//         e->icfls(v);
//         TRACE_("%zu ", v.size());
//         v.clear();
//      }
//   }
//#endif
//   TRACE ("\b] %s", b ? "found" : "no");
//   if (b) DEBUG ("c15u: explore: %s: j: %s", explore_stat(t, d).c_str(), j.str().c_str());
//   return b;
//}
//
//bool C15unfolder::find_alternative_only_last (const Config &c, const Disset &d, Cut &j)
//{
//   // - (complete but unoptimal)
//   // - consider the last (unjustified) event added to D, call it e
//   // - if you find some immediate conflict e' of e that is compatible with C (that
//   //   is, e' is not in conflict with any event in proc-max(C)), then set J = [e']
//   //   and return it
//   // - in fact we set J = C \cup [e'], because of the way we need to compute
//   //   the sleeping threads to pass them to steroids
//   // - as an optimization to avoid some SSB executions, you could skip from the
//   //   previous iteration those e' in D, as those will necessarily be blocked
//   // - if you don't find any such e', return false
//
//   Event * e;
//   bool b;
//#ifdef CONFIG_STATS_DETAILED
//   unsigned count = 0;
//#endif
//
//   // D is not empty
//   ASSERT (d.unjustified.begin() != d.unjustified.end());
//
//   // statistics
//   counters.alt.calls_built_comb++;
//   counters.alt.calls_explore_comb++;
//   counters.alt.spikes.sample (1); // number of spikes
//
//   // last event added to D
//   e = *d.unjustified.begin();
//   DEBUG ("c15u: alt: only-last: c %s e %s", c.str().c_str(), e->suid().c_str());
//
//   // scan the spike of that guy, we use 1 spike in the comb
//   comb.clear();
//   comb.add_spike (e);
//#ifdef CONFIG_STATS_DETAILED
//   counters.alt.spikesizeunfilt.sample (comb[0].size());
//#endif
//   b = false;
//   for (Event *ee : comb[0])
//   {
//#ifdef CONFIG_STATS_DETAILED
//      count++;
//#endif
//      if (!ee->flags.ind and !ee->in_cfl_with (c) and !d.intersects_with (ee))
//      {
//         j = c;
//         j.unionn (ee);
//         b = true;
//         break;
//      }
//   }
//#ifdef CONFIG_STATS_DETAILED
//   counters.alt.spikesizefilt.sample (count);
//#endif
//   return b;
//}
//
//bool C15unfolder::find_alternative_kpartial (const Config &c, const Disset &d, Cut &j)
//{
//   // We do an exahustive search for alternatives to D after C, we will find one
//   // iff one exists. We use a comb that has one spike per unjustified event D.
//   // Each spike is made out of the immediate conflicts of that event in D. The
//   // unjustified events in D are all enabled in C, none of them is in cex(C).
//
//   unsigned i, num_unjust;
//   std::vector<Event*> solution;
//
//#ifdef VERB_LEVEL_DEBUG
//   DEBUG_ ("c15u: alt: kpartial: k %u c %s d.unjust [",
//         kpartial_bound, c.str().c_str());
//   for (auto e : d.unjustified) DEBUG_("%p ", e);
//#endif
//   DEBUG ("\b]");
//
//   ASSERT (altalgo == Altalgo::OPTIMAL or
//         altalgo == Altalgo::KPARTIAL);
//   ASSERT (altalgo != Altalgo::OPTIMAL or
//         kpartial_bound == UINT_MAX);
//   ASSERT (kpartial_bound >= 1);
//
//   // build the spikes of the comb; there are many other ways to select the
//   // interesting spikes much more interesting than this plain truncation ...
//   comb.clear();
//   num_unjust = 0;
//   for (const auto e : d.unjustified)
//   {
//      if (num_unjust < kpartial_bound) comb.add_spike (e);
//      num_unjust++;
//   }
//   ASSERT (! comb.empty());
//   DEBUG ("c15u: alt: kpartial: comb: initially:\n%s", comb.str().c_str());
//
//   // we have constructed a (non-empty) comb
//   counters.alt.calls_built_comb++;
//#ifdef CONFIG_STATS_DETAILED
//   for (auto &spike : comb) counters.alt.spikesizeunfilt.sample (spike.size());
//#endif
//
//   // remove from each spike those events whose local configuration includes
//   // some ujustified event in D, or in conflict with someone in C; the
//   // (expensive) check "d.intersects_with" could be avoided if we are computing
//   // optimal alternatives, as those events could never make part of a solution;
//   // however, if we are computing partial alternatives, the check is
//   // unavoidable
//   for (auto &spike : comb)
//   {
//      i = 0;
//      while (i < spike.size())
//      {
//         if (spike[i]->flags.ind or spike[i]->in_cfl_with(c) or
//               (altalgo != Altalgo::OPTIMAL and d.intersects_with (spike[i])))
//         {
//            spike[i] = spike.back();
//            spike.pop_back();
//         }
//         else
//            i++;
//      }
//      // if one spike becomes empty, there is no alternative
//      if (spike.empty()) return false;
//   }
//   DEBUG ("c15u: alt: kpartial: comb: after removing D and #(C), and bounding:\n%s",
//         comb.str().c_str());
//
//   // we have to explore the comb
//   counters.alt.calls_explore_comb++;
//   counters.alt.spikes.sample (num_unjust);
//#ifdef CONFIG_STATS_DETAILED
//   for (auto &spike : comb) counters.alt.spikesizefilt.sample (spike.size());
//#endif
//
//   // explore the comb, the combinatorial explosion could happen here
//   if (enumerate_combination (0, solution))
//   {
//      // we include C in J, it doesn't hurt and it will make the calls to
//      // j.unionn() run faster; an alternative could be to do j.clear(), but we
//      // cannot do it: we need C to be in J to compute sleepsets
//      j = c;
//
//      // we build a cut as the union of of all local configurations for events
//      // in the solution
//      for (auto e : solution) j.unionn (e);
//      return true;
//   }
//   return false;
//}
//
//bool C15unfolder::find_alternative_sdpor (Config &c, const Disset &d, Cut &j)
//{
//   Event * e;
//   bool b;
//   unsigned i, color;
//
//   // find alternatives for only the last event in D
//   b = find_alternative_only_last (c, d, j);
//   if (! b) return b;
//
//   // colorize all events in C
//   color = u.get_fresh_color();
//   c.colorize (color);
//
//   // scan J until we find some event in J enabled at C
//   //j.dump ();
//   //c.dump ();
//   for (i = 0; i < j.num_procs(); i++)
//   {
//      for (e = j[i]; e; e = e->pre_proc())
//      {
//         SHOW (e->str().c_str(), "s");
//         // if e is in C, then this is "too low"
//         if (e->color == color) break;
//         // skip events where the pre-proc is not in C
//         if (e->pre_proc() and e->pre_proc()->color != color) continue;
//         // skip events where the pre-other is not in C
//         if (e->pre_other() and e->pre_other()->color != color) continue;
//         // we found the right e
//         i = c.num_procs();
//         break;
//      }
//   }
//
//   // assert that e is enabled at C
//   ASSERT (e);
//   ASSERT (e->color != color);
//   ASSERT (e->pre_proc() == c.proc_max (e->pid()));
//   ASSERT (! e->pre_other() or e->pre_other()->color == color);
//   ASSERT (! e->in_cfl_with (c));
//
//   // and that it is not in D !!
//   ASSERT (! e->flags.ind);
//
//   // our alternative is J := C \cup {e}
//   j = c;
//   j.unionn (e); // for fun, comment out this line ;)
//   return true;
//}

void C15unfolder::report_init ()
{
   // fill the fields stored in the Unfolder base class
   ASSERT (exec);
   std::vector<std::string> myargv (exec->argv.begin(), exec->argv.end());
   std::vector<std::string> myenv (exec->environ.begin(), --(exec->environ.end()));

   report.dpuversion = CONFIG_VERSION;
   report.path = path;
   report.argv = myargv;
   report.environ = myenv;
   report.memsize = exec->config.memsize;
   report.defaultstacksize = exec->config.defaultstacksize;
   report.tracesize = exec->config.tracesize;
   report.optlevel = exec->config.optlevel;

   report.nr_exitnz = 0;
   report.nr_abort = 0;
   report.defects.clear ();

   // fill ours
   report.alt = (int) altalgo;
   report.kbound = kpartial_bound;
}

} // namespace dpu
