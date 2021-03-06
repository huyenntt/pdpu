
#include "pes/config.hh"

namespace dpu {

void Config::dump () const
{
   PRINT ("Conf: dump: %p", this);
   PRINT ("== begin config =="); 
   __dump_cut ();
   PRINT ("conf: dump: mutex");
   __dump_mutexes ();
   PRINT ("== end config =="); 
}

void Config::__dump_mutexes () const
{
   for (auto &pair : mutexmax)
   {
      PRINT ("Offset %-#16lx e %08x", pair.first, pair.second->uid());
   }
}

} // namespace
