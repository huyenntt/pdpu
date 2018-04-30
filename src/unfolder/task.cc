/*
 * task.cc
 *
 *  Created on: Apr 25, 2018
 *      Author: admin
 */
#include "task.hh"

namespace dpu{

//Task:: Task() :
//      unfolder(),
//      dis ()
//{
//   //nothing
//}

Task:: Task(Disset d, stid::ExecutorConfig c) :
      unfolder(c),
      dis (d)
{
   //nothing
}

Task:: Task (const Task &other) : //copy constructor
      unfolder (other.unfolder),
      dis (other.dis)
{
   // Copy constructor
}


//Task:: Task(Replay &rpl, const Disset &dis, const Cut &j, Altalgo algo, const stid::ExecutorConfig &config)
//{
//   // Phai xem lai noi dung cua cai nay
//   unsigned tid;
////   exec = new stid::Executor(exe->m, config);
////   exec->set_replay (replay);
//
//   // no need for sleepsets if we are optimal
//   if (algo == Altalgo::OPTIMAL) return;
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
//         tid = replay.pidmap.get(e->pid());
//         TRACE_ ("r%u (#%u) %p; ", tid, e->pid(), (void*) e->action.addr);
//         exec->add_sleepset (tid, (void*) e->action.addr);
//      }
//   }
//   TRACE ("");
//}

void Task:: setup_exec(Replay &rpl, const Disset &dis, const Cut &j, Altalgo algo)
{
   unsigned tid;

     unfolder._exec->set_replay (rpl);

     // no need for sleepsets if we are optimal
     if (algo == Altalgo::OPTIMAL) return;

     // otherwise we set sleeping the thread of every unjustified event in D that
     // is still enabled at J; this assumes that J contains C
     TRACE_ ("c15u: explore: sleep set: ");
     unfolder._exec->clear_sleepset();
     for (auto e : dis.unjustified)
     {
        ASSERT (e->action.type == ActionType::MTXLOCK);
        if (! j.ex_is_cex (e))
        {
           tid = rpl.pidmap.get(e->pid());
           TRACE_ ("r%u (#%u) %p; ", tid, e->pid(), (void*) e->action.addr);
           unfolder._exec->add_sleepset (tid, (void*) e->action.addr);
        }
     }
     TRACE ("");
}

} // end of namespace




