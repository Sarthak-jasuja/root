/// \file ROOT/RClusterPool.hxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2020-03-11
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2020, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RClusterPool
#define ROOT7_RClusterPool

#include <ROOT/RCluster.hxx>
#include <ROOT/RNTupleUtil.hxx>

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <future>
#include <thread>
#include <set>
#include <vector>

namespace ROOT {
namespace Internal {
class RPageSource;
}

namespace Experimental {
namespace Internal {

// clang-format off
/**
\class ROOT::Experimental::Internal::RClusterPool
\ingroup NTuple
\brief Managed a set of clusters containing compressed and packed pages

The cluster pool steers the preloading of (partial) clusters. There is a two-step pipeline: in a first step,
compressed pages are read from clusters into a memory buffer. The second pipeline step decompresses the pages
and pushes them into the page pool. The actual logic of reading and unzipping is implemented by the page source.
The cluster pool only orchestrates the work queues for reading and unzipping. It uses one extra I/O thread for
reading waits for data from storage and generates no CPU load.

The unzipping step of the pipeline therefore behaves differently depending on whether or not implicit multi-threading
is turned on. If it is turned off, i.e. in a single-threaded environment, the cluster pool will only read the
compressed pages and the page source has to uncompresses pages at a later point when data from the page is requested.
*/
// clang-format on
class RClusterPool {
private:
   /// Request to load a subset of the columns of a particular cluster.
   /// Work items come in groups and are executed by the page source.
   struct RReadItem {
      /// Items with different bunch ids are scheduled for different vector reads
      std::int64_t fBunchId = -1;
      std::promise<std::unique_ptr<RCluster>> fPromise;
      RCluster::RKey fClusterKey;
   };

   /// Clusters that are currently being processed by the pipeline.  Every in-flight cluster has a corresponding
   /// work item, first a read item and then an unzip item.
   struct RInFlightCluster {
      std::future<std::unique_ptr<RCluster>> fFuture;
      RCluster::RKey fClusterKey;

      bool operator ==(const RInFlightCluster &other) const {
         return (fClusterKey.fClusterId == other.fClusterKey.fClusterId) &&
                (fClusterKey.fPhysicalColumnSet == other.fClusterKey.fPhysicalColumnSet);
      }
      bool operator !=(const RInFlightCluster &other) const { return !(*this == other); }
      /// First order by cluster id, then by number of columns, than by the column ids in fColumns
      bool operator <(const RInFlightCluster &other) const;
   };

   /// Every cluster pool is responsible for exactly one page source that triggers loading of the clusters
   /// (GetCluster()) and is used for implementing the I/O and cluster memory allocation (PageSource::LoadClusters()).
   ROOT::Internal::RPageSource &fPageSource;
   /// The number of clusters before the currently active cluster that should stay in the pool if present
   /// Reserved for later use.
   unsigned int fWindowPre = 0;
   /// The number of clusters that are being read in a single vector read.
   unsigned int fClusterBunchSize;
   /// Used as an ever-growing counter in GetCluster() to separate bunches of clusters from each other
   std::int64_t fBunchId = 0;
   /// The cache of clusters around the currently active cluster
   std::vector<std::unique_ptr<RCluster>> fPool;

   /// Protects the shared state between the main thread and the I/O thread, namely the work queue and the in-flight
   /// clusters vector
   std::mutex fLockWorkQueue;
   /// The clusters that were handed off to the I/O thread
   std::vector<RInFlightCluster> fInFlightClusters;
   /// Signals a non-empty I/O work queue
   std::condition_variable fCvHasReadWork;
   /// The communication channel to the I/O thread
   std::deque<RReadItem> fReadQueue;

   /// The I/O thread calls RPageSource::LoadClusters() asynchronously.  The thread is mostly waiting for the
   /// data to arrive (blocked by the kernel) and therefore can safely run in addition to the application
   /// main threads.
   std::thread fThreadIo;

   /// Every cluster id has at most one corresponding RCluster pointer in the pool
   RCluster *FindInPool(ROOT::DescriptorId_t clusterId) const;
   /// Returns an index of an unused element in fPool; callers of this function (GetCluster() and WaitFor())
   /// make sure that a free slot actually exists
   size_t FindFreeSlot() const;
   /// The I/O thread routine, there is exactly one I/O thread in-flight for every cluster pool
   void ExecReadClusters();
   /// Returns the given cluster from the pool, which needs to contain at least the columns `physicalColumns`.
   /// Executed at the end of GetCluster when all missing data pieces have been sent to the load queue.
   /// Ideally, the function returns without blocking if the cluster is already in the pool.
   RCluster *WaitFor(ROOT::DescriptorId_t clusterId, const RCluster::ColumnSet_t &physicalColumns);

public:
   static constexpr unsigned int kDefaultClusterBunchSize = 1;
   RClusterPool(ROOT::Internal::RPageSource &pageSource, unsigned int clusterBunchSize);
   explicit RClusterPool(ROOT::Internal::RPageSource &pageSource) : RClusterPool(pageSource, kDefaultClusterBunchSize)
   {
   }
   RClusterPool(const RClusterPool &other) = delete;
   RClusterPool &operator =(const RClusterPool &other) = delete;
   ~RClusterPool();

   /// Returns the requested cluster either from the pool or, in case of a cache miss, lets the I/O thread load
   /// the cluster in the pool, blocks until done, and then returns it.  Triggers along the way the background loading
   /// of the following fWindowPost number of clusters.  The returned cluster has at least all the pages of
   /// `physicalColumns` and possibly pages of other columns, too.  If implicit multi-threading is turned on, the
   /// uncompressed pages of the returned cluster are already pushed into the page pool associated with the page source
   /// upon return. The cluster remains valid until the next call to GetCluster().
   RCluster *GetCluster(ROOT::DescriptorId_t clusterId, const RCluster::ColumnSet_t &physicalColumns);

   /// Used by the unit tests to drain the queue of clusters to be preloaded
   void WaitForInFlightClusters();
}; // class RClusterPool

} // namespace Internal
} // namespace Experimental
} // namespace ROOT

#endif
