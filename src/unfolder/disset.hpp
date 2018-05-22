
Disset::Disset () :
   stack (),
   just (nullptr),
   unjust (nullptr),
   top_disabler (-1),
   top_idx (-1),
   ssb_count (0)
{
   stack.reserve (CAPACITY);
}

Disset:: Disset (const Disset &other) :
   just (other.just),
   unjust (other.unjust), // Xu ly phia duoi
   top_disabler (other.top_disabler),
   top_idx (other.top_idx),
   ssb_count (other.ssb_count)
{
//   PRINT ("Dis:cctor:");
   stack.reserve (CAPACITY);
   stack = other.stack;

   // Con phai set up next va prev cho tung element
//   PRINT ("dis:cctor: other stack");
//   for (auto &e : other.stack)
//         PRINT (" %p", (void*) &e);

//   PRINT ("dis:cctor: this->stack");
//   for (auto &e : stack)
//            PRINT (" %p", (void*) &e);
   // Moi element chi point den 1 Event duy nhat -> co the dung Event de xac dinh just, unjust va prev, next

   // Set up unjust and just pointers to new

   if (just)
      for (auto &ele : stack)
      {
         if (ele.e == other.just->e)
         {
//            PRINT ("dis: cctor: Just assignment");
            just = &ele;
            break;
         }
      }

//   PRINT ("dis: cctor: just pointer: %p", just);

   if (unjust)
      for (auto &ele : stack)
      {
         if (ele.e == other.unjust->e)
         {
            unjust = &ele;
            break;
         }
      }
//   PRINT ("dis: cctor: set unjust pointer: %p", unjust);

   // After getting head for just and unjust, set justified and unjustified lists
//   justified = Ls (just);
//   unjustified = Ls (unjust);


//   // set justified list
//   PRINT ("dis: cctor: set unjustified list");
   Elem *ele, *el;
   if (unjust)
   {
      ele = unjust;
      for (el = other.unjust; el; el = el->next)
      {
         for (auto &es : stack)
            if ((el->next) && (es.e == el->next->e))
            {
               ele->next = &es;
               es.prev  = ele; // Xem lai phan prev
            }
         ele = ele->next;
      }
   }

//   for (auto it = unjustified.begin(), end = unjustified.end();
//                  it != end; ++it) PRINT ("%p ", it);

//   PRINT ("dis: cctor: set justified list");
   // set justified list
   if (just)
   {
      ele = just;
      for (el = other.just; el; el = el->next)
      {
         for (auto &es : stack)
            if ((el->next) && (es.e == el->next->e))
               {
                  ele->next = &es;
                  // Xem lai phan prev
               }
         ele = ele->next;
      }
   }

//   for (auto it = justified.begin(), end = justified.end();
//               it != end; ++it) PRINT ("%p ", it);

//   PRINT ("dis: cctor: other.unjust: %p", (void*) other.unjust);
//   PRINT ("dis: cctor: unjust: %p", (void*) unjust);
//   PRINT ("dis:cctor: Done"); // Phai xem lai phan copy disset
}
// Thuc ra cai nay ko giup ich gi nhieu. Van phai copy stack va set up lai toan bo các elee trong stack.
Disset:: Disset (const Disset &&other) :
//   stack (std::move(other.stack)),
   just (std::move(other.just)),
   unjust (std::move(other.unjust)), // Xu ly phia duoi
   top_disabler (std::move(other.top_disabler)),
   top_idx (std::move(other.top_idx)),
//   justified (std::move(other.justified)),
//   unjustified (std::move(other.unjustified)), // ko can 2 cau lenh nay
   ssb_count (std::move(other.ssb_count))
{
//   PRINT ("dis: mctor: stack");
   stack.reserve (CAPACITY);
   stack = other.stack;
//   stack = std::move(other.stack); // sao ko giu nguyen vung nho cua other.stack??? -> ko giữ dc do phải reserve new memory location

// Moi element chi point den 1 Event duy nhat -> co the dung Event de xac dinh just, unjust va prev, next

      // Set up unjust and just pointers to new

      if (just)
         for (auto &ele : stack)
         {
            if (ele.e == other.just->e)
            {
//               PRINT ("dis: cctor: Just assignment");
               just = &ele;
               break;
            }
         }
//      PRINT ("dis: mctor: just pointer: %p", just);

      if (unjust)
         for (auto &ele : stack)
         {
            if (ele.e == other.unjust->e)
            {
               unjust = &ele;
               break;
            }
         }
//      PRINT ("dis: mctor: set unjust pointer: %p", unjust);

      // After getting head for just and unjust, set justified and unjustified lists
   //   justified = Ls (just);
   //   unjustified = Ls (unjust);

      // set justified list
//      PRINT ("dis: mctor: set unjustified list");
/*
 * Unjust is head but it will be the last event in stack.
 * unjust->next points to the event added in the unjustied list.
 */
      Elem *ele, *el;

      if (unjust)
      {
         ele = unjust;
         for (el = other.unjust; el; el = el->next)
         {
            for (auto &es : stack)
               if ((el->next) && (es.e == el->next->e))
               {
                  ele->next = &es;
                  es.prev  = ele; // Xem lai phan prev
               }
            ele = ele->next;
         }

//         for (auto it = unjustified.begin(), end = unjustified.end();
//              it != end; ++it)
//            PRINT ("%p ", it);
      }

//      PRINT ("dis: cctor: set justified list");
      // set justified list
      if (just)
      {
         ele = just;
         for (el = other.just; el; el = el->next)
         {
            for (auto &es : stack)
               if ((el->next) && (es.e == el->next->e))
                  {
                     ele->next = &es;
                     // Xem lai phan prev : Cai nay ko dung den prev nen ko can set
                  }
            ele = ele->next;
         }

//         for (auto it = justified.begin(), end = justified.end();
//                           it != end; ++it) PRINT ("%p ", it);
      }


   // Khong can phai set up lai tat ca cac pointer nua???
//   for (auto &e : stack) PRINT ("%p ", (void*) &e);
}

void Disset::just_push (Elem *e)
{
   ASSERT (e);
   ASSERT (! just_contains (e->e));
   e->next = just;
   just = e;
}

Disset::Elem *Disset::just_pop ()
{
   Elem *e;

   ASSERT (just);
   e = just;
   just = just->next;
   return e;
}

bool Disset::just_isempty ()
{
   return just == nullptr;
}

inline bool Disset::just_contains (Event *e) const
{
   const Elem *el;
   for (el = just; el; el = el->next)
      if (el->e == e) return true;
   return false;
}

Disset::Elem *Disset::just_peek ()
{
   ASSERT (just);
   return just;
}

inline bool Disset::unjust_contains (Event *e) const
{
   const Elem *el;
   for (el = unjust; el; el = el->next)
      if (el->e == e) return true;
   return false;
}

void Disset::unjust_add (Elem *e)
{
   // we add e to the head of the list

   ASSERT (e);
   ASSERT (! unjust_contains (e->e));
   e->next = unjust;
   e->prev = nullptr;
   if (unjust) unjust->prev = e;
   unjust = e;
}

void Disset::unjust_remove (Elem *e)
{
   // we extract e from the list of unjustified events

   ASSERT (e);
   ASSERT (unjust_contains (e->e));
   if (e->prev)
      e->prev->next = e->next;
   else
   {
//      PRINT ("dis: unjust_remove: ụnjust: %p, e: %p", unjust, e);
      ASSERT (unjust == e);
      unjust = e->next;
   }
   if (e->next) e->next->prev = e->prev;
}

void Disset::unjust_remove_head ()
{
   ASSERT (unjust);
   ASSERT (unjust->prev == nullptr);
   unjust = unjust->next;
   if (unjust) unjust->prev = nullptr;
}

bool Disset::unjust_isempty ()
{
   return unjust == nullptr;
}

void Disset::add (Event *e, int idx)
{
   // we add e to the stack, add it to the list of unjustified events and update
   // the top_idx variable

   ASSERT (e);
//   PRINT ("Dis: add: ");
   ASSERT (!e->flags.ind); // e is not in D yet
   ASSERT (idx >= 0);
   ASSERT (idx >= top_idx);
   if (stack.size() >= stack.capacity())
      throw std::out_of_range ("Dis: capacity exceeded");

   e->flags.ind = 1;
//   PRINT ("Dis: add: e->flags.ind after %d",e->flags.ind);
   stack.push_back ({.e = e, .idx = idx, .disabler = -1});
   unjust_add (&stack.back()); // Loi o day
   top_idx = idx;
//   PRINT ("Dis: add: e->flags.ind %d", stack.back().e->flags.ind);
}

void Disset::unadd ()
{
   // removes from D the last event inserted; it must be in the unjust list
//   PRINT ("c15: explore: unadding......");

   ASSERT (stack.size ());
   ASSERT (unjust == &stack.back());
   ASSERT (stack.back().e->flags.ind);

   stack.back().e->flags.ind = 0;
   unjust_remove_head ();
   stack.pop_back ();
   top_idx = stack.size() ? stack.back().idx : -1;
}

bool Disset::trail_push (Event *e, int idx)
{
   Elem *el, *nxt;

   ASSERT (e);
   ASSERT (idx >= 0);

   // if we are pushing to the trail an event that is already in D, we got a
   // sleep-set block execution, and we should stop it
   if (e->flags.ind)
   {
      ssb_count++;

#ifdef VERB_LEVEL_TRACE
      unsigned u, j;
      u = j = 0;
      for (auto it = justified.begin(), end = justified.end();
            it != end; ++it) j++;
      for (auto it = unjustified.begin(), end = unjustified.end();
            it != end; ++it) u++;

      TRACE ("c15u: disset: SSB, count %u, |trail| %u, "
            "|D| %u (%u just, %u unjust)",
            ssb_count, idx, u + j, j, u);
#endif

      return false;
   }

   // iterate through the list of unjustified events and move to the list of
   // justified events those that get justified by event e
   for (el = unjust; el; el = nxt)
   {
      nxt = el->next;
      if (e->in_icfl_with (el->e))
      {
         DEBUG ("dis: unadd: justifying %08x (disabler %08x, idx %d)",
               el->e->uid(), e->uid(), idx);
         unjust_remove (el);
         just_push (el);
         el->disabler = idx;
         top_disabler = idx;
      }
   }
   return true;
}

void Disset::trail_pop (int idx)
{
   // if the last event popped out of the trail, at depth idx, was strictly
   // deeper than top_idx (the top element in the stack D), then we need to
   // remove at least the top, and potentially other elements of D, as multiple
   // events in D can have been stored at the same depth top_idx;
   ASSERT (idx >= -1);
//   PRINT ("Dis: trail_pop: idx %u, top_idx %d, top_disabler %d", idx, top_idx, top_disabler);
//   if (!stack.empty())
//      PRINT ("dis: trail_pop: stack.back:ind %d", stack.back().e->flags.ind);
//   else
//      PRINT ("dis: trail_pop: empty disset");

   while (idx < top_idx) // Voi nhung event in trail (idx < top_idx), remove e khoi D vi moi e in trail: e < e' with e'in D
   {
      ASSERT (idx == top_idx - 1);
//      ASSERT (stack.back().e->flags.ind); // Because the last event is changed
//      ind by dis::unadd function, this statement should be removed
      unjust_remove (&stack.back());
      stack.back().e->flags.ind = 0;

//      DEBUG ("c15u: disset: removing %08x", stack.back().e->uid());
      PRINT ("dis: trail_pop removing %08x", stack.back().e->uid());
      stack.pop_back ();
      top_idx = stack.size() ? stack.back().idx : -1;
   }

   // removing one event from the trail means that some events that were so far
   // justified could now become unjustified; those events will be in the list
   // of justified events and will be such that their "disabler" field will be
   // equal to idx; we iterate through them and transfer them from one list to
   // the other
   while (idx <= top_disabler)
   {
      ASSERT (idx == top_disabler);
      ASSERT (! just_isempty ());
      ASSERT (idx == just_peek()->disabler);
      PRINT ("dis: trail_pop: un-justifying %08x", just_peek()->e->uid());
      unjust_add (just_pop ());
      top_disabler = just_isempty () ? -1 : just_peek()->disabler;
   }
}

bool Disset::intersects_with (const Event *e) const
{
   const Elem *el;
   for (el = unjust; el; el = el->next)
      if (el->e->is_predeq_of (e)) return true;
   return false;
}

bool Disset::intersects_with (const Cut &c) const
{
   unsigned i;
   for (i = 0; i < c.num_procs(); i++)
      if (c[i] and intersects_with (c[i])) return true;
   return false;
}
