#!/usr/bin/env python

import os

_LOGDIR_ = "logs"

class bcolors:
        HEADER = '\033[95m'
        OKBLUE = '\033[94m'
        OKGREEN = '\033[92m'
        WARNING = '\033[93m'
        FAIL = '\033[91m'
        ENDC = '\033[0m'
        BOLD = '\033[1m'
        UNDERLINE = '\033[4m'

def getFiles():
        global _LOGDIR_
        return os.listdir( _LOGDIR_ )

def filenameToManip( f ):
        n = f.split( ".")[0]
        p = n.split( "_" )
        i = p[-1][-1]
        k = p[-2][-1]
        c = p[-3][-1]
        exp = "".join(p[:-3])
        return (exp, c, k, i)

def manipToFilename( exp, c, k, i ):
        return exp + "_core" + str( c ) + "_alt" + str( k ) + "_i" + str( i ) + ".txt"

def extractManips( fl ):
        manips = {}
        for f in fl:
                (exp, c, k, i) = filenameToManip( f )
                if( manips.has_key( exp ) ):
                        if manips[ exp ].has_key( k ):
                                manips[ exp ][ k ].append( (c, i ) )
                        else:
                                manips[ exp ][ k ] = [ ( c, i ) ]
                else:
                        manips[exp] = { k : None }
                        manips[exp][k] = [ ( c, i ) ]
        return manips

def extractStats( fn ):
        global _LOGDIR_
        fd = open( _LOGDIR_ + "/" + fn )
        lines = fd.readlines()
        ln = lines[-2]
        if not ln.startswith( "dpu: por: summary:" ):
                return None
        parts = (ln.split( ":" )[-1]).split()
        prev = None
        for p in parts:
                if p.startswith( "defects" ):
                        defects = int( prev )
                if p.startswith( "max-configs" ):
                        maxconfig = int( prev )
                if p.startswith( "SSBs" ):
                        ssbs = int( prev )
                if p.startswith( "events" ):
                        events = int( prev )
                prev = p
        
        fd.close()
        return defects, maxconfig, ssbs, events

def getResults( manips ):
        results = {}
        for bench, manip in manips.iteritems():
                if not results.has_key( bench ):
                        results[ bench ] = {}
                for k, runs in manip.iteritems():
                        if not results[bench].has_key( k ):
                                results[bench][k] = []
                        for (cores, i) in runs:
                                fn = manipToFilename( bench, cores, k, i )
                                stats = extractStats( fn )
                                results[bench][k].append( ( stats, fn ) )
                                #if not None is stats:
                                #        print stats, fn
        return results

def compareResults( results ):
        for bench, manips in results.iteritems():
                for k, data_f in manips.iteritems():
                        defects = []
                        maxconfig = []
                        ssbs = []
                        events = []
                        for r in data_f:
                                ( stats, filename ) = r
                                if not None is stats:
                                        defects.append( stats[0] )
                                        maxconfig.append( stats[1] )
                                        ssbs.append( stats[2] )
                                        events.append( stats[3] )
                        print bcolors.HEADER, " * = * = Bench", bench, "k = ", k, ": ", len(defects), "experiments", bcolors.ENDC
                        ok = True
                        d_s = set( defects )
                        if len( d_s ) != 1:
                                ok = False
                                print bcolors.WARNING, "Nb of defects are different:", bcolors.ENDC, list( d_s ) 
                        m_s = set( maxconfig )
                        if len( m_s ) != 1:
                                ok = False
                                print bcolors.WARNING, "Nb of maximum config are different:", bcolors.ENDC, list( m_s )
                        s_s = set( ssbs )
                        if len( s_s ) != 1:
                                ok = False
                                print bcolors.WARNING, "Nb of SSBs are different:", bcolors.ENDC, list( s_s )
                        e_s = set( events )
                        if len( e_s ) != 1:
                                ok = False
                                print bcolors.WARNING, "Nb of events are different:", bcolors.ENDC, list( e_s )
                        if ok == False:
                                print bcolors.FAIL, "=> There was a problem with bench", bench, "for k = ", k, bcolors.ENDC
                        else:
                                print bcolors.OKGREEN, "=> Ok for bench", bench, "for k = ", k, bcolors.ENDC, d_s.pop(), m_s.pop(), s_s.pop(), e_s.pop()
                                

def main():
        files = getFiles()
        manips = extractManips( files )
        results = getResults( manips )
        compareResults( results )
        
        
	return

if __name__ == "__main__":
	main()

