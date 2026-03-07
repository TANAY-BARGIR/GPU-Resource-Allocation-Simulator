#ifndef SEGMENT_TREE_H
#define SEGMENT_TREE_H

#include <vector>
#include <utility>
#include <string>

using namespace std;

/**
 * Segment Tree for contiguous GPU memory block allocation.
 *
 * Each node tracks:
 *   prefixFree  – longest free run from the left edge
 *   suffixFree  – longest free run from the right edge
 *   maxFree     – longest contiguous free run anywhere in the segment
 *   totalLen    – length of the segment (constant after build)
 *
 * Supports O(log N) first-fit allocation and deallocation.
 */
class SegmentTree {
public:
    explicit SegmentTree(int totalUnits);

    // Find a contiguous free block of `size` units (first-fit).
    // Returns the starting index, or -1 if no fit exists.
    int  allocate(int size);

    // Free the block starting at `start` of length `size`.
    void deallocate(int start, int size);

    // Largest contiguous free block in the entire range.
    int  largestFreeBlock() const;

    // Total number of free units (sum).
    int  totalFree() const;

    // Total capacity.
    int  capacity() const;

    // Returns a list of (start, length) pairs for every free segment.
    vector<pair<int,int>> getFreeSegments() const;

    // Returns a list of (start, length) pairs for every used segment.
    vector<pair<int,int>> getUsedSegments() const;

private:
    struct Node {
        int prefixFree;
        int suffixFree;
        int maxFree;
        int totalLen;
    };

    int n_;                    // capacity (rounded to next power of 2 internally)
    int realN_;                // actual capacity requested
    vector<Node> tree_;
    vector<bool> used_;   // ground-truth per-unit status

    void build(int node, int start, int end);
    void update(int node, int start, int end, int l, int r, bool markUsed);
    int  queryFirstFit(int node, int start, int end, int size) const;

    // Helpers for segment enumeration
    void collectFreeSegments(vector<pair<int,int>>& out) const;
    void collectUsedSegments(vector<pair<int,int>>& out) const;
};

#endif // SEGMENT_TREE_H
