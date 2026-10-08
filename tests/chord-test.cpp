#include "../src/ChordState.h"
#include <cassert>
int main(){
 ChordState s;
 assert(!s.Process(false,true,true).trigger); // PgUp alone
 s.Process(true,false,true); // End pressed after PgUp
 assert(!s.Process(false,true,true).trigger); // repeat must not activate
 s.Process(false,true,false);
 auto a=s.Process(false,true,true);assert(a.trigger&&a.suppress);
 a=s.Process(false,true,true);assert(!a.trigger&&a.suppress); // hold repeat
 s.Process(true,false,false); // End released first
 a=s.Process(false,true,false);assert(!a.trigger&&a.suppress);
 a=s.Process(false,true,true);assert(!a.trigger&&!a.suppress);
 s.Process(false,true,false);
 s.Process(true,false,true);
 assert(s.Process(false,true,true).trigger);
 s.Process(false,true,false);
 assert(s.Process(false,true,true).trigger); // second distinct press
}
