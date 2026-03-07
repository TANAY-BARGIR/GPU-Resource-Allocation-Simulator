#include "SegmentTree.h"
#include <algorithm>

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

SegmentTree::SegmentTree(int totalUnits)
    : realN_(totalUnits), used_(totalUnits, false) {
  n_ = totalUnits;
  tree_.resize(4 * n_);
  build(1, 0, n_ - 1);
}

void SegmentTree::build(int node, int start, int end) {
  int len = end - start + 1;
  tree_[node] = {len, len, len, len};
  if (start == end)
    return;
  int mid = (start + end) / 2;
  build(2 * node, start, mid);
  build(2 * node + 1, mid + 1, end);
}

// ─────────────────────────────────────────────────────────────────────────────
// Update — mark a range [l, r] as used (markUsed=true) or free (false)
// ─────────────────────────────────────────────────────────────────────────────

void SegmentTree::update(int node, int start, int end, int l, int r,
                         bool markUsed) {
  if (r < start || end < l)
    return;

  if (start == end) {
    int val = markUsed ? 0 : 1;
    tree_[node] = {val, val, val, 1};
    return;
  }

  int mid = (start + end) / 2;
  update(2 * node, start, mid, l, r, markUsed);
  update(2 * node + 1, mid + 1, end, l, r, markUsed);

  // Merge children
  auto &left = tree_[2 * node];
  auto &right = tree_[2 * node + 1];
  int len = end - start + 1;

  tree_[node].totalLen = len;

  // Prefix: extends into left child; if left is entirely free, extends into
  // right
  tree_[node].prefixFree = left.prefixFree;
  if (left.prefixFree == left.totalLen)
    tree_[node].prefixFree += right.prefixFree;

  // Suffix: extends into right child; if right is entirely free, extends into
  // left
  tree_[node].suffixFree = right.suffixFree;
  if (right.suffixFree == right.totalLen)
    tree_[node].suffixFree += left.suffixFree;

  // Max free: best of left, right, or the crossing boundary
  tree_[node].maxFree = max(
      {left.maxFree, right.maxFree, left.suffixFree + right.prefixFree});
}

// ─────────────────────────────────────────────────────────────────────────────
// First-fit query (internal helper, kept for potential future use).
// ─────────────────────────────────────────────────────────────────────────────

int SegmentTree::queryFirstFit(int node, int start, int end, int size) const {
  if (tree_[node].maxFree < size)
    return -1;
  if (start == end)
    return start; // single unit, and it's free

  int mid = (start + end) / 2;

  // Try left child first
  if (tree_[2 * node].maxFree >= size)
    return queryFirstFit(2 * node, start, mid, size);

  // Try the crossing boundary (left suffix + right prefix)
  int crossStart = mid - tree_[2 * node].suffixFree + 1;
  if (tree_[2 * node].suffixFree + tree_[2 * node + 1].prefixFree >= size)
    return crossStart;

  // Try right child
  return queryFirstFit(2 * node + 1, mid + 1, end, size);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public interface  —  Best-Fit allocation strategy
// ─────────────────────────────────────────────────────────────────────────────

int SegmentTree::allocate(int size) {
  if (size <= 0 || size > n_)
    return -1;

  // Quick check: is there any contiguous block large enough?
  if (tree_[1].maxFree < size)
    return -1;

  // Best-fit: find the smallest free segment that accommodates `size`
  auto freeSegs = getFreeSegments();
  int bestPos = -1;
  int bestLen = n_ + 1; // larger than any real segment

  for (auto &[segStart, segLen] : freeSegs) {
    if (segLen >= size && segLen < bestLen) {
      bestPos = segStart;
      bestLen = segLen;
    }
  }

  if (bestPos == -1)
    return -1;

  // Mark units as used
  for (int i = bestPos; i < bestPos + size; ++i)
    used_[i] = true;
  update(1, 0, n_ - 1, bestPos, bestPos + size - 1, true);
  return bestPos;
}

void SegmentTree::deallocate(int start, int size) {
  if (start < 0 || start + size > n_)
    return;
  for (int i = start; i < start + size; ++i)
    used_[i] = false;
  update(1, 0, n_ - 1, start, start + size - 1, false);
}

int SegmentTree::largestFreeBlock() const { return tree_[1].maxFree; }

int SegmentTree::totalFree() const {
  int cnt = 0;
  for (int i = 0; i < realN_; ++i)
    if (!used_[i])
      ++cnt;
  return cnt;
}

int SegmentTree::capacity() const { return realN_; }

// ─────────────────────────────────────────────────────────────────────────────
// Segment enumeration (for visualization)
// ─────────────────────────────────────────────────────────────────────────────

void SegmentTree::collectFreeSegments(
    vector<pair<int, int>> &out) const {
  int i = 0;
  while (i < realN_) {
    if (!used_[i]) {
      int start = i;
      while (i < realN_ && !used_[i])
        ++i;
      out.emplace_back(start, i - start);
    } else {
      ++i;
    }
  }
}

void SegmentTree::collectUsedSegments(
    vector<pair<int, int>> &out) const {
  int i = 0;
  while (i < realN_) {
    if (used_[i]) {
      int start = i;
      while (i < realN_ && used_[i])
        ++i;
      out.emplace_back(start, i - start);
    } else {
      ++i;
    }
  }
}

vector<pair<int, int>> SegmentTree::getFreeSegments() const {
  vector<pair<int, int>> segs;
  collectFreeSegments(segs);
  return segs;
}

vector<pair<int, int>> SegmentTree::getUsedSegments() const {
  vector<pair<int, int>> segs;
  collectUsedSegments(segs);
  return segs;
}
