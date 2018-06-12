
#include "unfolder/disset.hh"


namespace dpu
{

void Disset:: set_flags()
{
   for (auto &ele : stack)
   {
//      omp_set_lock(&ele.e->elock);
         ele.e->flags.ind = 1;
//      omp_unset_lock(&ele.e->elock);
   }
}

void Disset:: unset_flags()
{
//   PRINT ("disset: unset_flags: ...");

   for (auto &ele : stack)
   {
//       omp_set_lock(&ele.e->elock);
          ele.e->flags.ind = 0;
//       omp_unset_lock(&ele.e->elock);
   }
}

void Disset::dump () const
{
   const Elem *e;

   PRINT ("== begin disset =="); 

   PRINT ("%zu events, top-idx %d, top-disabler %d, ssb-count %u",
         stack.size(), top_idx, top_disabler, ssb_count);
   PRINT ("Unjustified:");
   for (e = unjust; e; e = e->next)
   {
      PRINT  (" ele %p idx %d %s", (void*) e, e->idx, e->e->str().c_str());
   }

   PRINT ("Justified (top-down):");
   for (e = just; e; e = e->next)
   {
      PRINT (" ele %p idx %d dis %d %s", (void*) e, e->idx, e->disabler, e->e->str().c_str());
   }

   PRINT ("== end disset =="); 
}

} // namespace
