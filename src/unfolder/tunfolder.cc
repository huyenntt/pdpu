/*
 * tunfolder.cc
 *
 *  Created on: Apr 27, 2018
 *      Author: admin
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>

#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/Format.h>

#include "stid/executor.hh"
#include "pes/process.hh"
#include "unfolder/tunfolder.hh" // must be before verbosity.h

#include "misc.hh"
#include "verbosity.h"
#include "opts.hh"

namespace dpu
{
// Nho set up them cac thong so khac giong c15
Tunfolder::Tunfolder (const stid::ExecutorConfig &config, Altalgo a, unsigned kbound, unsigned maxcts) :
   _exec (nullptr),
   _m (nullptr),
   _config (config),
   alt_algo (a),
   kpartial_bound (kbound),
   comb (a, kbound),
   max_context_switches (maxcts)
{
   DEBUG ("Tunf: ctor");
   DEBUG ("Tunf: config: %zu", _config.memsize);
//   unsigned i;

//   // initialize the start array (this is an invariant expected and maintained
//   // by stream_to_events)
//   for (i = 0; i < Unfolding::MAX_PROC; i++)
//      StreamConverter<T>::start[i] = nullptr; // Phai xem lai phan nay, no co can thiet cho unfolding trong c15 hay ko?
}

//Tunfolder:: Tunfolder (const Tunfolder &&other) :
//      _exec (std::move(other._exec)),
//      _context (std::move(other._context)), // Load_bitcode se set up context
//      _path (std::move(other._path)),
//      _m (std::move(other._m)),
//      _config (std::move(other._config))
//{
//   PRINT ("Tunf.cctor");
//   DEBUG ("Tunf: config: %zu", _config.memsize);
//}

//Tunfolder:: Tunfolder (const Tunfolder &other) :
//      _exec (other._exec),
//      _context (other._context), // Load_bitcode se set up context
//      _path (other._path),
//      _m (other._m),
//      _config (other._config)
//{
//   PRINT ("Tunf.cctor");
//   DEBUG ("Tunf: config: %zu", _config.memsize);
//}

Tunfolder::~Tunfolder ()
{
   DEBUG ("tunf.dtor: this %p", this);
   delete _exec;
}

void Tunfolder::_load_bitcode (std::string &&filepath)
{
   llvm::SMDiagnostic err;
   std::string errors;

   ASSERT (filepath.size());
   ASSERT (_path.size() == 0);
   ASSERT (_exec == 0);
   ASSERT (_m == 0);
//   _path = std::move (filepath);
   _path = filepath;
   PRINT ("tunf: loadbitcode: path: %s, filepath %s", _path.c_str(), filepath.c_str());

   // necessary for the JIT engine; we should move this elsewhere
   static bool init = false;
   if (not init)
   {
      init = true;
      llvm::InitializeNativeTarget();
      llvm::InitializeNativeTargetAsmPrinter();
      llvm::InitializeNativeTargetAsmParser();
   }

   // parse the .ll file and get a Module out of it
   std::unique_ptr<llvm::Module> mod (llvm::parseIRFile (_path, err, _context));
   _m = mod.get();

   // if errors found, report and terminate
   if (! mod.get ()) {
      llvm::raw_string_ostream os (errors);
      err.print (_path.c_str(), os);
      os.flush ();
      PRINT ("tunf: load-bytecode: '%s': %s\n", _path.c_str(), errors.c_str());
      throw std::invalid_argument (errors);
   }

   // print external symbols
//   if (verb_trace) print_external_syms ("dpu: "); // Se xem xet sau

//   PRINT ("dpu: unf: O%u-optimization + jitting...", opts::optlevel); // Cai nay thuoc ve C15
   PRINT ("tunf: load-bytecode: setting up the bytecode executor...");
   try {
      _exec = new stid::Executor (std::move (mod), _config);
   } catch (const std::exception &e) {
      PRINT ("tunf: load-bytecode: errors preparing the bytecode executor");
      PRINT ("tunf: load-bytecode: %s", e.what());
      throw e;
   }
   PRINT ("tunf: load-bytecode: executor successfully created!");

//   PRINT ("instpath.size: %lu", opts::instpath.size());
   // Thuc ra cai nay store_bitcode chi can thiet khi minh muon luu lai bitcode (co le de backup)
   if (opts::instpath.size())  // Opts trong namespace opts in opts.hh
   {
      PRINT ("tunf: load-bytecode: saving instrumented bytecode to %s",
         opts::instpath.c_str());
      _store_bitcode (opts::instpath.c_str()); // Neu can thi chua bitcode bang function nay
   }

   PRINT ("tunf: load-bytecode: Done!");
}

void Tunfolder::_store_bitcode (const std::string &filename) const
{
   int fd = open (filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
   ASSERT (fd >= 0);
   llvm::raw_fd_ostream f (fd, true);
   f << *_m;
}

void Tunfolder::_print_external_syms (const char *prefix)
{
   std::string str;
   llvm::raw_string_ostream os (str);
   std::vector<std::pair<llvm::StringRef,llvm::Type*>> funs;
   std::vector<std::pair<llvm::StringRef,llvm::Type*>> globs;
   size_t len;

   ASSERT (_m);
   if (! prefix) prefix = "";

   // functions
   len = 0;
   for (llvm::Function &f : _m->functions())
   {
      if (not f.isDeclaration()) continue;
      funs.emplace_back (f.getName(), f.getType());
      if (len < funs.back().first.size())
         len = funs.back().first.size();
   }
   std::sort (funs.begin(), funs.end());
   os << prefix << "External functions:\n";
   for (auto &pair : funs)
   {
      os << prefix << llvm::format ("  %-*s ", len, pair.first.str().c_str());
      os << *pair.second << "\n";
   }

   // global variables
   len = 0;
   for (llvm::GlobalVariable &g : _m->globals())
   {
      if (not g.isDeclaration()) continue;
      globs.emplace_back (g.getName(), g.getType());
      if (len < globs.back().first.size())
         len = globs.back().first.size();
   }
   std::sort (globs.begin(), globs.end());
   os << prefix << "External variables:\n";
   for (auto &pair : globs)
   {
      os << prefix << llvm::format ("  %-*s ", len, pair.first.str().c_str());
      os << *pair.second << "\n";
   }
   os.flush ();
   PRINT ("%s", str.c_str());
}

void Tunfolder::_set_args (std::vector<const char *> argv)
{
   PRINT ("tunf: set-args: |argv| %zu", argv.size());
   _exec->argv = argv;
}

void Tunfolder::_set_env (std::vector<const char *> env)
{
   if (env.empty() or env.back() != nullptr)
      env.push_back (nullptr);
   PRINT ("tunf: set-env: |env| %zu", env.size());
   _exec->environ = env;
}

void Tunfolder::_set_default_environment ()
{
   char * const * v;
   std::vector<const char *> env;

   // make a copy of our environment
   for (v = environ; *v; ++v) env.push_back (*v); // environ la cai gi???
   env.push_back (nullptr);
//   PRINT ("unf: set-env: |env| %zu", env.size());
   _exec->environ = env;
   PRINT ("tunf: set-env: |env| %zu", _exec->environ.size());
}

// Ham nay de lam gi nhi? Tai sao ko có stream_to_events???
//Config Tunfolder::_add_one_run (const stid::Replay &r)
//{
//   Config c (Unfolding::MAX_PROC);
//
//   ASSERT (_exec);
//
//   // run the guest
//   DEBUG ("unf: add-1-run: this %p |replay| %zu", this, r.size());
//   _exec->set_replay (r);
//   DEBUG ("unf: add-1-run: running the guest ...");
//   _exec->run ();
//
//   // get a stream object from the executor and transform it into events
//   stid::action_streamt actions (_exec->get_trace ());
//   //actions.print ();
//   //actions.print ();
//   StreamConverter<T>::convert (actions, c);
//   return c;
//}

bool Tunfolder::_is_conflict_free (const std::vector<Event *> &sol,
   const Event *e) const
{
   // we return false iff e is in conflict with some event of the partial solution "sol"
   int i;
   for (i = 0; i < sol.size(); i++)
      if (e->in_cfl_with (sol[i])) return false;
   return true;
}

void  Tunfolder:: _set_replay_sleepset (Replay &replay, const Disset &d, const Cut &j)
{
     unsigned tid;
     PRINT ("tunf: Set_replay_sleepset: set replay");
     _exec->set_replay (replay);

     if (opts::alt_algo == Altalgo::OPTIMAL) return;

     //  // otherwise we set sleeping the thread of every unjustified event in D that
     //  // is still enabled at J; this assumes that J contains C
     PRINT ("tunf: Set replay and sleep set: set sleep set ");
     _exec->clear_sleepset();
//     d.dump();

     PRINT ("tunfolder: set sleep set");
     for (auto e : d.unjustified)
     {
        ASSERT (e->action.type == ActionType::MTXLOCK);
        PRINT ("e: %s", e->str().c_str());
        if (! j.ex_is_cex (e)) // J nay se xem lay o dau?
        {
           tid = replay.pidmap.get(e->pid());
//           TRACE_ ("r%u (#%u) %p; ", tid, e->pid(), (void*) e->action.addr);
//           PRINT ("r%u (#%u) %p; ", tid, e->pid(), (void*) e->action.addr);
//           _exec->add_sleepset (tid, (void*) e->action.addr);
           /*Phan nay co ve van phai de nguyen*/
//           DEBUG ("r%u (#%u) %0*x; ", tid, e->pid(), e->action.offset);
           _exec->add_sleepset (tid, (void*) e->action.offset);
        }
     }
     TRACE ("");
//     PRINT ("tunf: set_replay_sleepset: Done");
}

/*All functions related to algorithm */

//void Tunfolder::compute_cex_lock (Event *e, Event **head)
//{
//   // 1. let ep be the pre-proc of e
//   // 2. let em be the pre-mem of e
//   //
//   // 3. if em <= ep, then return   // there is no cex
//   // 4. em = em.pre-em             // em is now a LOCK
//   // 5. if (em <= ep) return       // because when you fire [ep] the lock is acquired!
//   // 6. em = em.pre-mem            // em is now an UNLOCK
//   // 7. insert event (e.action, ep, em)
//   // 8. goto 3
//
//   Event *ep, *em, *ee;
//
//   PRINT ("c15u: cex-lock: starting from %s", e->str().c_str());
//   ASSERT (e)
//   ASSERT (e->action.type == ActionType::MTXLOCK);
//
//   // 1,2: ep/em are the predecessors in process/memory
//   ep = e->pre_proc();
//   em = e->pre_other();
//
//   while (1)
//   {
//      // 3. we are done if we got em <= ep (em == null means em = bottom!)
//      if (!em or em->is_predeq_of (ep)) return;
//
//      // 4,5: jump back 1 predecessor in memory, if we got em <= ep, return
//      ASSERT (em->action.type == ActionType::MTXUNLK);
//      em = em->pre_other();
//      ASSERT (em);
//      ASSERT (em->action.type == ActionType::MTXLOCK);
//      if (em->is_predeq_of (ep)) return;
//
//      // 6. back 1 more predecessor
//      em = em->pre_other();
//      ASSERT (!em or em->action.type == ActionType::MTXUNLK);
//
//      // 7. (action, ep, em) is a possibly new event
////      omp_unset_lock(&wlock);
//         ee = u.event (e->action, ep, em); // Cau lenh nay
////      omp_unset_lock(&wlock);
//      PRINT ("c15u: cex-lock:  new cex: %s", ee->str().c_str());
//
//      // we add it to the linked-list
//      ee->next = *head; // next is also used in cut_to_replay
//      *head = ee;
//   }
//}

//void Tunfolder::compute_cex (Config &c, Event **head) // Cho nay chua dong cham den u
//{
//   Event *e;
//
//   PRINT ("c15u: cex: c %p *head %p |mm| %lu", &c, *head, c.mutexmax.size());
//
//   for (auto const & max : c.mutexmax)
//   {
//      // skip events that are not locks or unlocks
//      e = max.second;
//      PRINT ("Event: %s", e->str().c_str());
//      ASSERT (e);
//      if (e->action.type != ActionType::MTXUNLK and
//            e->action.type != ActionType::MTXLOCK) continue;
//
//      // scan the lock chain backwards
//      for (; e; e = e->pre_other())
//      {
//         if (e->action.type == ActionType::MTXLOCK)
//         {
//            compute_cex_lock (e, head); // Ham nay se them event vào u
//         }
//      }
//   }
//}

bool Tunfolder::enumerate_combination (unsigned i,
   std::vector<Event*> &sol)
{
   // We enumerate combinations using one event from each spike
   // - the partial solution is stored in sol
   // - if sol is conflict-free, we return true

   ASSERT (i < comb.size());
   for (auto e : comb[i])
   {
      if (! _is_conflict_free (sol, e)) continue;
      sol.push_back (e);
      if (sol.size() == comb.size()) return true;
      if (enumerate_combination (i+1, sol)) return true;
      sol.pop_back ();
   }
   return false;
}

bool Tunfolder::might_find_alternative (Config &c, Disset &d, Event *e)
{
   // this method can return return false only if we are totally sure that no
   // alternative to D \cup e exists after C; this method should run fast, it
   // will be called after popping every single event event from the trail; our
   // implementation is very fast: we return false if the event type is != lock

   // e->icfls() is nonempty => e is a lock
   ASSERT (! e->icfl_count() or e->action.type == ActionType::MTXLOCK);

   return e->action.type == ActionType::MTXLOCK;
}

bool Tunfolder::find_alternative (const Trail &t, Config &c, const Disset &d, Cut &j, Unfolding &u)
{
   bool b;

//   PRINT ("find_alt: alt_algo: %u", alt_algo);

   switch (alt_algo) {
   case Altalgo::OPTIMAL :
   case Altalgo::KPARTIAL :
      b = find_alternative_kpartial (c, d, j);
      break;
   case Altalgo::ONLYLAST :
      b = find_alternative_only_last (c, d, j);
      break;
   case Altalgo::SDPOR :
      b = find_alternative_sdpor (c, d, j, u);
      break;
   }

   // no alternative may intersect with d
   if (b)
   {
      //if (d.intersects_with (j)) { d.dump (); j.dump (); }
      ASSERT (! d.intersects_with (j));
   }

   TRACE_ ("c15u: explore: %s: alt: [", _explore_stat (t, d).c_str());
//#ifdef VERB_LEVEL_TRACE
//   if (verb_trace)
//   {
      std::vector<Event*> v;
      for (auto e : d.unjustified)
      {
         e->icfls(v);
         TRACE_("%zu ", v.size());
         v.clear();
      }
//   }
//#endif
   TRACE ("\b] %s", b ? "found" : "no");
//   if (b) DEBUG ("c15u: explore: %s: j: %s", _explore_stat(t, d).c_str(), j.str().c_str());

     if (b) PRINT ("c15u: explore: %s: j: %s", _explore_stat(t, d).c_str(), j.str().c_str());

   return b;
}

bool Tunfolder::find_alternative_only_last (const Config &c, const Disset &d, Cut &j)
{
   // - (complete but unoptimal)
   // - consider the last (unjustified) event added to D, call it e
   // - if you find some immediate conflict e' of e that is compatible with C (that
   //   is, e' is not in conflict with any event in proc-max(C)), then set J = [e']
   //   and return it
   // - in fact we set J = C \cup [e'], because of the way we need to compute
   //   the sleeping threads to pass them to steroids
   // - as an optimization to avoid some SSB executions, you could skip from the
   //   previous iteration those e' in D, as those will necessarily be blocked
   // - if you don't find any such e', return false

   Event * e;
   bool b;
#ifdef CONFIG_STATS_DETAILED
   unsigned count = 0;
#endif

   // D is not empty
   ASSERT (d.unjustified.begin() != d.unjustified.end());

   // statistics: Tam gac lai, cai nay chi co trong c15 de thong ke thoi. Bo qua.
//   counters.alt.calls_built_comb++;
//   counters.alt.calls_explore_comb++;
//   counters.alt.spikes.sample (1); // number of spikes

   // last event added to D
   e = *d.unjustified.begin();
   DEBUG ("c15u: alt: only-last: c %s e %s", c.str().c_str(), e->suid().c_str());

   // scan the spike of that guy, we use 1 spike in the comb
   comb.clear();
   comb.add_spike (e);
   PRINT ("comb: %s",comb.str().c_str());
#ifdef CONFIG_STATS_DETAILED
//   counters.alt.spikesizeunfilt.sample (comb[0].size());
#endif
   b = false;
   for (Event *ee : comb[0])
   {
      #ifdef CONFIG_STATS_DETAILED
            count++;
      #endif

//      if (!ee->flags.ind and !ee->in_cfl_with (c) and !d.intersects_with (ee))
      if ( !d.inD (ee) and !ee->in_cfl_with (c) and !d.intersects_with (ee))
      {
         j = c;
         PRINT("J============================C================================");
         j.dump();
         j.unionn (ee);

         b = true;
         break;
      }
   }
#ifdef CONFIG_STATS_DETAILED
//   counters.alt.spikesizefilt.sample (count);
#endif
   return b;
}

bool Tunfolder::find_alternative_kpartial (const Config &c, const Disset &d, Cut &j)
{
   // We do an exhaustive search for alternatives to D after C, we will find one
   // iff one exists. We use a comb that has one spike per unjustified event D.
   // Each spike is made out of the immediate conflicts of that event in D. The
   // unjustified events in D are all enabled in C, none of them is in cex(C).

   unsigned i, num_unjust;
   std::vector<Event*> solution;

#ifdef VERB_LEVEL_DEBUG
   DEBUG_ ("c15u: alt: kpartial: k %u c %s d.unjust [",
         kpartial_bound, c.str().c_str());
   for (auto e : d.unjustified) DEBUG_("%p ", e);
#endif
   DEBUG ("\b]");

   ASSERT (alt_algo == Altalgo::OPTIMAL or
         alt_algo == Altalgo::KPARTIAL);
   ASSERT (alt_algo != Altalgo::OPTIMAL or
         kpartial_bound == UINT_MAX);
   ASSERT (kpartial_bound >= 1);

   // build the spikes of the comb; there are many other ways to select the
   // interesting spikes much more interesting than this plain truncation ...
   comb.clear();
   num_unjust = 0;
   for (const auto e : d.unjustified)
   {
      if (num_unjust < kpartial_bound) comb.add_spike (e);
      num_unjust++;
   }
   ASSERT (! comb.empty());
   DEBUG ("c15u: alt: kpartial: comb: initially:\n%s", comb.str().c_str());

   // we have constructed a (non-empty) comb
//   counters.alt.calls_built_comb++;
#ifdef CONFIG_STATS_DETAILED
//   for (auto &spike : comb) counters.alt.spikesizeunfilt.sample (spike.size());
#endif

   // remove from each spike those events whose local configuration includes
   // some ujustified event in D, or in conflict with someone in C; the
   // (expensive) check "d.intersects_with" could be avoided if we are computing
   // optimal alternatives, as those events could never make part of a solution;
   // however, if we are computing partial alternatives, the check is
   // unavoidable
   for (auto &spike : comb)
   {
      i = 0;
      while (i < spike.size())
      {
//         if (spike[i]->flags.ind or spike[i]->in_cfl_with(c) or
         if (d.inD(spike[i]) or spike[i]->in_cfl_with(c) or
               (alt_algo != Altalgo::OPTIMAL and d.intersects_with (spike[i])))
         {
            spike[i] = spike.back();
            spike.pop_back();
         }
         else
            i++;
      }
      // if one spike becomes empty, there is no alternative
      if (spike.empty()) return false;
   }
   DEBUG ("c15u: alt: kpartial: comb: after removing D and #(C), and bounding:\n%s",
         comb.str().c_str());

   // we have to explore the comb
//   counters.alt.calls_explore_comb++;
//   counters.alt.spikes.sample (num_unjust);
#ifdef CONFIG_STATS_DETAILED
//   for (auto &spike : comb) counters.alt.spikesizefilt.sample (spike.size());
#endif

   // explore the comb, the combinatorial explosion could happen here
   if (enumerate_combination (0, solution))
   {
      // we include C in J, it doesn't hurt and it will make the calls to
      // j.unionn() run faster; an alternative could be to do j.clear(), but we
      // cannot do it: we need C to be in J to compute sleepsets
      j = c;

      // we build a cut as the union of of all local configurations for events
      // in the solution
      for (auto e : solution) j.unionn (e);
      return true;
   }
   return false;
}

bool Tunfolder::find_alternative_sdpor (Config &c, const Disset &d, Cut &j, Unfolding &u) // add u
{
   Event * e;
   bool b;
   unsigned i, color;

   // find alternatives for only the last event in D
   b = find_alternative_only_last (c, d, j);
   if (! b) return b;

   // colorize all events in C
   color = u.get_fresh_color(); // dung cham den u nay. Cai nay chi read from u nen ko co van de gi lon.

   c.colorize (color); // touche u -> need a lock here

   // scan J until we find some event in J enabled at C
   //j.dump ();
   //c.dump ();
   for (i = 0; i < j.num_procs(); i++)
   {
      for (e = j[i]; e; e = e->pre_proc())
      {
         SHOW (e->str().c_str(), "s");
         // if e is in C, then this is "too low"
         if (e->color == color) break;
         // skip events where the pre-proc is not in C
         if (e->pre_proc() and e->pre_proc()->color != color) continue;
         // skip events where the pre-other is not in C
         if (e->pre_other() and e->pre_other()->color != color) continue;
         // we found the right e
         i = c.num_procs();
         break;
      }
   }

   // assert that e is enabled at C
   ASSERT (e);
   ASSERT (e->color != color);
   ASSERT (e->pre_proc() == c.proc_max (e->pid()));
   ASSERT (! e->pre_other() or e->pre_other()->color == color);
   ASSERT (! e->in_cfl_with (c));

   // and that it is not in D !!
//   ASSERT (! e->flags.ind);
   ASSERT (! d.inD(e));

   // our alternative is J := C \cup {e}
   j = c;
   j.unionn (e); // for fun, comment out this line ;)
   return true;
}

std::string Tunfolder::_explore_stat (const Trail &t, const Disset &d) const
{
   unsigned u, j;

   u = j = 0;
   for (auto it = d.justified.begin(), end = d.justified.end();
         it != end; ++it) j++;
   for (auto it = d.unjustified.begin(), end = d.unjustified.end();
         it != end; ++it) u++;

   // 37e  9j  8u
   return fmt ("%2ue %2uj %2uu", t.size(), j, u);
}
// void Tunfolder:: _get_por_analysis ()
//{
//   PRINT ("tunf: get_por_analysis: inpath: %s", opts::inpath.c_str());
//   _load_bitcode (std::string (opts::inpath));
//
//   // set values for the argv and environ variables
//   _set_args (opts::argv);
//   _set_default_environment();
//}

} // namespace dpu





