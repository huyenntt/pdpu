
#include "unfolder/disset.hh"


namespace dpu
{

void Disset:: set_flags()
{
//   PRINT ("disset: set_flags: ...");
//   if (!stack.empty())
//   {
////      omp_set_lock(&stack.back().e->elock);
//         stack.back().e->flags.ind = 1;
////      omp_unset_lock(&stack.back().e->elock);
//   }
   for (auto &ele : stack)
   {
      omp_set_lock(&ele.e->elock); // lock event in disset until we finish the computation of alternative
         ele.e->flags.ind = 1;
//      omp_unset_lock(&ele.e->elock);
   }
   // Only last event added to D needs to be set flags.ind = 1 as it will be changed back later to
//   if (unjust)
//   {
//      omp_set_lock(&unjust->e->elock);
//         unjust->e->flags.ind = 1;
//      omp_unset_lock(&unjust->e->elock);
//   }
}

void Disset:: unset_flags()
{
//   PRINT ("disset: unset_flags: ...");
   for (auto &ele : stack)
   {
//       omp_set_lock(&ele.e->elock);
          ele.e->flags.ind = 0;
       omp_unset_lock(&ele.e->elock);
   }
//   if (unjust)
//   {
//      omp_set_lock(&unjust->e->elock);
//               unjust->e->flags.ind = 0;
//      omp_unset_lock(&unjust->e->elock);
//   }
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
