#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>
#include "Util.h"
#include "Core.h"

class Partitions {
  public:
    Partitions(const std::shared_ptr<std::unordered_map<unsigned long, std::unordered_set<PatchID>>> partitionable);
  private:
    std::unordered_map<unsigned long, std::unordered_set<Partition>> idToPartitions;
};
