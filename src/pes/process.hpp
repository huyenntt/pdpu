
Process::Process (Event *creat, Unfolding *u) :
   counters {1},
   u (u)
{
   //DEBUG ("Process.ctor: this %p pid %d sizeof %d", this, pid(), sizeof (Process));

   // construct the first event box
   Eventbox *b = first_box ();
   new (b) Eventbox (0);

   // construct a THSTART event in the first box
   Event *e = b->event_above ();
   new (e) Event (creat, true);

   // initialize the pointer to the last event created
   last = e;

   //initialize plock
   omp_init_lock(&plock);
}

Process:: ~Process ()
{
   omp_destroy_lock(&plock);
}

Process::It<Event> Process::begin ()
{
   return Process::It<Event> (first_event());
}
Process::It<Event> Process::end ()
{
   return Process::It<Event> (last + 1);
}

const Process::It<const Event> Process::begin () const
{
   return Process::It<const Event> (first_event());
}
const Process::It<const Event> Process::end () const
{
   return Process::It<const Event> (last + 1);
}


unsigned Process::pid () const
{
   return Unfolding::ptr2pid (this);
}
size_t Process::offset (void *ptr) const
{
   return ((size_t) ptr) - (size_t) this;
}

Event *Process::first_event () const
{
   return first_box()->event_above();
}

Eventbox *Process::first_box () const
{
   return ((Eventbox *) (this + 1));
}

/// THSTART, 0 process predecessor, 1 predecessor (THCREAT) in another process
Event *Process::add_event_0p (Event *creat)
{
   Event *e;

   // lock the process for only THSTART as this process is a new one.
   omp_set_lock(&plock);

   ASSERT (creat); // insertion of bottom is done elsewhere
   ASSERT (last); // we have a last
   ASSERT (pid() == last->pid()); // last point inside us
   ASSERT (! last->flags.boxlast); // box should be open

   // check for available space
   have_room ();

   if (last == creat->cone[pid()])
   {
      // add one event at the end of the pool
      e = last + 1;
      new (e) Event (creat, false);
   }
   else
   {
      // close the box, add a new box, insert the THSTART
      last->flags.boxlast = 1;
      Eventbox *b = new (last + 1) Eventbox (creat->cone[pid()]);
      e = b->event_above ();
      new (e) Event (creat, true);
      ASSERT (e->flags.boxfirst);
   }

   // update the last pointer and the counters
   last = e;
   counters.events++;

//   // unlock the process
   omp_unset_lock(&plock);
   return e;
}

/// THCREAT(tid), THEXIT(), one predecessor (in the process)
Event *Process::add_event_1p (Action ac, Event *p)
{
   Event *e;

//   omp_set_lock(&plock);

   ASSERT (last); // we have a last
   ASSERT (pid() == last->pid()); // last point inside us
   ASSERT (! last->flags.boxlast); // box should be open

   // check for available space
   have_room ();

   if (p == last)
   {
      // add one event at the end of the pool
      e = last + 1;
      new (e) Event (ac, false);
   }
   else
   {
      // create one box and add the event
      last->flags.boxlast = 1;
      Eventbox *b = new (last + 1) Eventbox (p);
      e = b->event_above ();
      new (e) Event (ac, true);
      ASSERT (e->flags.boxfirst);
   }

   counters.events++;
   last = e;

   //unlock the process
//   omp_unset_lock(&plock);

   return e;
}

/// THJOIN(tid), MTXLOCK(addr), MTXUNLK(addr), two predecessors (process, memory/exit)
Event * Process::add_event_2p (Action ac, Event *p, Event *m)
{
   Event *e;

   // lock the process to add new event
//   omp_set_lock(&plock);

   ASSERT (last); // we have a last
   ASSERT (pid() == last->pid()); // last point inside us
   ASSERT (! last->flags.boxlast); // box should be open

   // check for available space
   have_room ();

   if (p == last)
   {
      // add one event at the end of the pool
      e = last + 1;
      new (e) Event (ac, m, false);
   }
   else
   {
      // create one box and add the event
      last->flags.boxlast = 1;
      Eventbox *b = new (last + 1) Eventbox (p);
      e = b->event_above ();
      new (e) Event (ac, m, true);
      ASSERT (e->flags.boxfirst);
   }

   counters.events++;
   last = e;

   // unlock the process
//   omp_unset_lock(&plock);

   return e;
}

void Process::have_room () const
{
   // check that we have enough memory
   // this->last points to the last event
   // last+1 is the base address of the new one; last+3 is the upper limit of
   // one event + one box
   static_assert (sizeof (Event) >= sizeof (Eventbox), "Event should be larger than Eventbox");
   if (offset (last + 3) >= Unfolding::PROC_SIZE)
   {
      throw std::range_error (fmt
            ("Process %d: failure to add new events: out of memory", pid ()));
   }
}

/// returns the memory size of the data (indirectly) pointed by fields in this object
size_t Process::pointed_memory_size () const
{
   size_t size;

   size = 0;
   for (const Event &e : *this)
      size += e.pointed_memory_size();
   return size;
}
