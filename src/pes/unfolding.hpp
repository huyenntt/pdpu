
// Unfolding::Unfolding () in pes/unfolding.cc

Unfolding::Unfolding (const Unfolding &&other)
{
   // FIXME - to be implemented
   ASSERT (0);
}

Unfolding::~Unfolding ()
{
   DEBUG ("Unfolding.dtor: this %p", this);
   free (procs);
}

Process *Unfolding::proc (unsigned p) const
{
   return (Process *) (procs + p * PROC_SIZE);
}

Event *Unfolding::find1 (Action *ac, Event *p)
{
   ASSERT (p);
   ASSERT (p->node[0].post.size() <= 1);

   if (p->node[0].post.size() == 0) return nullptr;
   ASSERT (ac->type == p->node[0].post[0]->action.type);
   return p->node[0].post[0];
}

Event *Unfolding::find2 (Action *ac, Event *p, Event *m)
{
#ifdef CONFIG_DEBUG
   // exactly one event with preset {p,m} ...
   int count = 0;
   for (Event *e : p->node[0].post)
   {
      if (e->pre_other() == m) count++;
   }
   ASSERT (count <= 1);
#endif

   // FIXME - we could choose here p or m depending on the size of the post
   for (Event *e : p->node[0].post)
   {
      if (e->pre_other() == m)
      {
         // ... and that event is action *ac
//         PRINT ("e: %s", e->str().c_str());
//         PRINT ("ac: %s, addr %p offset %lu", action_type_str (ac->type), (void*) ac->addr, ac->offset);
         ASSERT (e->action == *ac);
         return e;
      }
   }
   return nullptr;
}

unsigned Unfolding::get_fresh_color ()
{
   color++;
   return color;
}
