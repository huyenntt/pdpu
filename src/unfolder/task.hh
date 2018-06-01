/*
 * task.hh
 *
 *  Created on: Apr 25, 2018
 *      Author: admin
 */

#ifndef SRC_UNFOLDER_TASK_HH_
#define SRC_UNFOLDER_TASK_HH_

#include "unfolder/disset.hh"
#include "unfolder/replay.hh"
#include "unfolder/alt-algorithm.hh"
#include "unfolder/tunfolder.hh"

#include "pes/cut.hh"

//#include "stid/executor.hh"
namespace dpu {

//class Task {
//public:
//   Tunfolder unfolder;
//   Replay rep;
//   Disset dis;
////   Trail trail;
//public:
////   Task ();
//   Task (stid::ExecutorConfig &c, Replay &replay, const Disset &d, const Cut &j, Altalgo algo);
//   Task (const Task &other); //copy constructor
////   Task (const Task &&other); //move constructor
////   Task(Replay &rpl, const Disset &dis, const Cut &j, Altalgo algo);
//
//   void setup_exec(Replay &rpl, const Disset &dis, const Cut &j, Altalgo algo);
//   void dump();
//   void test_run();
//};

class Task
{
public:
//   int tskid;
//   Replay rep;
   Disset dis;
   Cut add;
   Trail trail;
   Config conf;

public:
//   Task ();
//   Task (Replay rpl, const Disset &d, const Cut &add, const Trail &t, const Config &c);
//   Task (int tcount, const Disset &d, const Cut &add, const Trail &t, const Config &c);
   Task (const Disset &d, const Cut &add, const Trail &t, const Config &c);
   Task (const Task &&other);
//   Task& operator= (Task &&other);
   void dump();

};
} // end of namespace
#endif /* SRC_UNFOLDER_TASK_HH_ */
