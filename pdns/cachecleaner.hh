/*
 * This file is part of PowerDNS or dnsdist.
 * Copyright -- PowerDNS.COM B.V. and its contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * In addition, for the avoidance of any doubt, permission is granted to
 * link this program with OpenSSL and to (re)distribute the binaries
 * produced as the result of such linking.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef PDNS_CACHECLEANER_HH
#define PDNS_CACHECLEANER_HH

// this function can clean any cache that has a getTTD() method on its entries, and a 'sequence' index as its second index
// the ritual is that the oldest entries are in *front* of the sequence collection, so on a hit, move an item to the end
// on a miss, move it to the beginning
template <typename T> void pruneCollection(T& collection, unsigned int maxCached, unsigned int scanFraction=1000)
{
  time_t now=time(0);
  unsigned int toTrim=0;
  
  unsigned int cacheSize=collection.size();

  if(cacheSize > maxCached) {
    toTrim = cacheSize - maxCached;
  }

//  cout<<"Need to trim "<<toTrim<<" from cache to meet target!\n";

  typedef typename T::template nth_index<1>::type sequence_t;
  sequence_t& sidx=collection.template get<1>();

  unsigned int tried=0, lookAt, erased=0;

  // two modes - if toTrim is 0, just look through 1/scanFraction of all records 
  // and nuke everything that is expired
  // otherwise, scan first 5*toTrim records, and stop once we've nuked enough
  if(toTrim)
    lookAt=5*toTrim;
  else
    lookAt=cacheSize/scanFraction;

  typename sequence_t::iterator iter=sidx.begin(), eiter;
  for(; iter != sidx.end() && tried < lookAt ; ++tried) {
    if(iter->getTTD() < now) { 
      sidx.erase(iter++);
      erased++;
    }
    else
      ++iter;

    if(toTrim && erased > toTrim)
      break;
  }

  //cout<<"erased "<<erased<<" records based on ttd\n";
  
  if(erased >= toTrim) // done
    return;

  toTrim -= erased;

  //if(toTrim)
    // cout<<"Still have "<<toTrim - erased<<" entries left to erase to meet target\n"; 

  eiter=iter=sidx.begin();
  std::advance(eiter, toTrim); 
  sidx.erase(iter, eiter);      // just lob it off from the beginning
}

// note: this expects iterator from first index, and sequence MUST be second index!
template <typename T> void moveCacheItemToFrontOrBack(T& collection, typename T::iterator& iter, bool front)
{
  typedef typename T::template nth_index<1>::type sequence_t;
  sequence_t& sidx=collection.template get<1>();
  typename sequence_t::iterator si=collection.template project<1>(iter);
  if(front)
    sidx.relocate(sidx.begin(), si); // at the beginning of the delete queue
  else
    sidx.relocate(sidx.end(), si);  // back
}

template <typename T> void moveCacheItemToFront(T& collection, typename T::iterator& iter)
{
  moveCacheItemToFrontOrBack(collection, iter, true);
}

template <typename T> void moveCacheItemToBack(T& collection, typename T::iterator& iter)
{
  moveCacheItemToFrontOrBack(collection, iter, false);
}

#endif
