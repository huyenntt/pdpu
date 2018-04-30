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

class Task {
public:
   Tunfolder unfolder;
   Disset dis;
//   Trail trail;
public:
   Task ();
   Task (Disset d, stid::ExecutorConfig c);
   Task (const Task &other); //copy constructor
//   Task(Replay &rpl, const Disset &dis, const Cut &j, Altalgo algo);

   void setup_exec(Replay &rpl, const Disset &dis, const Cut &j, Altalgo algo);
};


} // end of namespace
#endif /* SRC_UNFOLDER_TASK_HH_ */
