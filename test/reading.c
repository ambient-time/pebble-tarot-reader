#include "../src/c/reading.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
  unsigned seen[78]={0}, reverse=0;
  const int counts[]={1,3,5,6,7,10};
  for(unsigned seed=0;seed<10000;seed++) for(int m=0;m<6;m++) {
    Reading a,b;reading_draw(&a,counts[m],seed,true);reading_draw(&b,counts[m],seed,true);
    assert(reading_valid(&a));assert(!memcmp(&a,&b,sizeof(a)));
    if(m==5) {seen[a.cards[0]]++;reverse+=a.reversed[0];}
    a.revealed=(1u<<a.count)-1;assert(reading_valid(&a));
    a.revealed=1u<<a.count;assert(!reading_valid(&a));
    a=b;a.selected=a.count;assert(!reading_valid(&a));
    a=b;a.cards[0]=78;assert(!reading_valid(&a));
    if(a.count>1){a=b;a.cards[0]=a.cards[1];assert(!reading_valid(&a));}
    a=b;a.reversed[0]=2;assert(!reading_valid(&a));
    a=b;a.magic=0;assert(!reading_valid(&a));
    reading_draw(&a,counts[m],seed,false);assert(reading_valid(&a));
    for(int i=0;i<a.count;i++)assert(!a.reversed[i]);
  }
  for(int i=0;i<78;i++)assert(seen[i]>50 && seen[i]<220);
  assert(reverse>2500 && reverse<3100);
  Reading bad={0};assert(!reading_valid(&bad));
  reading_draw(&bad,255,0,true);assert(bad.count==1 && reading_valid(&bad));
  puts("PASS: 60,000 seeded spreads, no duplicates, reversals, all 78 reachable, persisted-state validation");
}
