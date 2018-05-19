/*
 * task.cc
 *
 *  Created on: Apr 25, 2018
 *      Author: admin
 */
#include "task.hh"

namespace dpu{

//Task:: Task() :
//      rep (),
//      dis ();
//{
//   PRINT ("Task.ctor()");
//}

//Task:: Task(Replay rpl, const Disset &d, const Cut &j, const Trail &t, const Config &c) :
//      rep (rpl),
//      dis (d),
//      add (j),
//      trail (t),
//      conf (c)
//{
//   PRINT ("task: ctor: (replay,dis,add, trail)");
//}

Task:: Task(const Disset &d, const Cut &j, const Trail &t, const Config &c) :
      dis (d),
      add (j),
      trail (t),
      conf (c)
{
   PRINT ("task: ctor: (replay,dis,add, trail)");
}


//Task:: Task (const Task &&other) :
//      rep (std::move(other.rep)),
//      dis (std::move(other.dis))
//{
//
//}

Task:: Task (const Task &&other) :
//      rep (std::move(other.rep)),
      dis (std::move(other.dis)),
      add (std::move(other.add)),
      trail (std::move(other.trail)),
      conf (std::move(other.conf))
{
   PRINT ("Task.mctor:Done");
   add.dump();
}

// move operator
//Task& Task:: operator= (Task &&other)
//{
//   rep = std::move(other.rep);
//   dis = std::move(other.dis);
//}

void Task:: dump()
{
   PRINT ("Task: Dumping task:");
//   PRINT ("Task: Replay: %s", rep.str().c_str()); // Chua ro in replay ra nhu the nao
//   PRINT ("tasks: dump: replay: %s", rep.str().c_str());
   PRINT ("task: dump: dis");
   dis.dump();
   PRINT ("task: dump: add");
   add.dump();
   PRINT ("task: dump: trail");
   trail.dump();
   PRINT ("task: dump: conf");
   conf.dump();
}

////Task:: Task() :
////      unfolder(),
////      dis ()
////{
////   //nothing
////}
//
//
//Task:: Task (stid::ExecutorConfig &c, Replay &replay, const Disset &d, const Cut &j, Altalgo algo) :
//      unfolder(c),
//      rep (replay),
//      dis (d)
//{
//   PRINT ("Task.ctor");
//   //nothing
//   DEBUG ("task: ctor: setup executor");
//   setup_exec(replay, d, j, algo);
//}
//
////Task:: Task (const Task &&other) : //copy constructor
////      unfolder (std::move(other.unfolder)),
////      rep (std::move(other.rep)),
////      dis (std::move(other.dis))
////{
////   PRINT ("Task.cctor");
////   // Copy constructor
////}
//
//Task:: Task (const Task &other) : //copy constructor
//      unfolder (other.unfolder),
//      rep (other.rep),
//      dis (other.dis)
//{
//   PRINT ("Task.cctor");
//   // Copy constructor
//}
//
//
//
//
////Task:: Task(Replay &rpl, const Disset &dis, const Cut &j, Altalgo algo, const stid::ExecutorConfig &config)
////{
////   // Phai xem lai noi dung cua cai nay
////   unsigned tid;
//////   exec = new stid::Executor(exe->m, config);
//////   exec->set_replay (replay);
////
////   // no need for sleepsets if we are optimal
////   if (algo == Altalgo::OPTIMAL) return;
////
////   // otherwise we set sleeping the thread of every unjustified event in D that
////   // is still enabled at J; this assumes that J contains C
////   TRACE_ ("c15u: explore: sleep set: ");
////   exec->clear_sleepset();
////   for (auto e : d.unjustified)
////   {
////      ASSERT (e->action.type == ActionType::MTXLOCK);
////      if (! j.ex_is_cex (e))
////      {
////         tid = replay.pidmap.get(e->pid());
////         TRACE_ ("r%u (#%u) %p; ", tid, e->pid(), (void*) e->action.addr);
////         exec->add_sleepset (tid, (void*) e->action.addr);
////      }
////   }
////   TRACE ("");
////}
//
//void Task:: setup_exec(Replay &rpl, const Disset &dis, const Cut &j, Altalgo algo)
//{
//  unsigned tid;
//  PRINT ("task: set up exec: load_bitcode, env, args");
//  unfolder._get_por_analysis();
//
//  PRINT ("task: set replay");
//  unfolder._exec->set_replay (rpl);
////  PRINT ("task: tunf: exec.replay %s",unfolder._exec->rt.replay.str().c_str());
//
//  // no need for sleepsets if we are optimal
//  if (algo == Altalgo::OPTIMAL) return;
//
//  // otherwise we set sleeping the thread of every unjustified event in D that
//  // is still enabled at J; this assumes that J contains C
//  DEBUG ("task: set up sleep set: ");
//  unfolder._exec->clear_sleepset();
//  for (auto e : dis.unjustified)
//  {
//     ASSERT (e->action.type == ActionType::MTXLOCK);
//     if (! j.ex_is_cex (e))
//     {
//        tid = rpl.pidmap.get(e->pid());
//        TRACE_ ("r%u (#%u) %p; ", tid, e->pid(), (void*) e->action.addr);
//        unfolder._exec->add_sleepset (tid, (void*) e->action.addr);
//     }
//  }
//  TRACE ("");
//  DEBUG ("Task: finish setting up exec");
////  test_run();
//}
//
//void Task:: test_run()
//{
//   PRINT ("task:: test_run: ");
//   unfolder._exec->run();
//}

} // end of namespace




