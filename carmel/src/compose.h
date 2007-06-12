#ifndef GRAEHL_CARMEL_COMPOSE_H
#define GRAEHL_CARMEL_COMPOSE_H
#include <graehl/shared/myassert.h>

#include <graehl/shared/2hash.h>



namespace graehl {

static const int EMPTY=0;

struct TrioKey {
  static unsigned int aMax;
  static unsigned int bMax;
  unsigned int aState;
  unsigned int bState;
  char filter;
  bool operator == (const TrioKey &t) const 
  {
    return (aState == t.aState) && (bState == t.bState)
      && (filter == t.filter);
  }
  TrioKey() {}
  TrioKey(int a, int b, char c) : 
    aState(a), bState(b), filter(c) {}
  size_t hash() const
  {
    Assert ( aState < aMax && bState < bMax);
	return uint32_hash((bMax * (filter*aMax + aState) + bState));
  }
};


struct HalfArcState {
  int dest;			// in unshared (lhs) transducer
  int source;			// in (rhs) transducer with the shared arcs
  int hiddenLetter;		// the matching letter that disappears in composition (index into lhs transducer's output alphabet)
  bool operator == ( const HalfArcState &s ) const
  {
    return ( dest == s.dest )
      && ( source == s.source )
      && ( hiddenLetter == s.hiddenLetter );
  }
  HalfArcState() {}
  HalfArcState(int a, int b, int c) :
    dest(a), source(b), hiddenLetter(c) {}
  size_t hash() const
  {
    return uint32_hash(TrioKey::bMax*(hiddenLetter*TrioKey::aMax + dest) + source);
  }
};

struct TrioID {
  int num;
  TrioKey tri;
};

}

BEGIN_HASH(graehl::TrioKey) {
  return x.hash();
  } END_HASH

  BEGIN_HASH(graehl::HalfArcState) {
      return x.hash();
  } END_HASH


#endif
