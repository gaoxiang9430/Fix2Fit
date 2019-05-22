#include <iostream>
#include <string>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include "Partitions.h"
#include "Core.h"

using std::unordered_set;
using std::unordered_map;
using std::shared_ptr;

Partitions::Partitions(const shared_ptr<unordered_map<unsigned long, unordered_set<PatchID>>> partitionable){
  for ( auto it = (*partitionable).begin(); it != (*partitionable).end(); ++it ){
    std::shared_ptr<std::vector<PatchID>> par(new std::vector<PatchID>);
    for (PatchID patchID: it->second)
      par->push_back(patchID);
    Partition partition;
    partition.patches = par;
    partition.id = it->first;
    unordered_set<Partition> partitions;
    partitions.insert(partition);
    idToPartitions[it->first] = partitions;
  }
}

