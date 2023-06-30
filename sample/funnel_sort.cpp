// Copyright (c) 2023 Giopler
// This code is licensed under the permissive MIT License (MIT).
// SPDX-License-Identifier: MIT-Modern-Variant
// https://fedoraproject.org/wiki/Licensing:MIT#Modern_Variants
//
// Permission is hereby granted, without written agreement and without
// license or royalty fees, to use, copy, modify, and distribute this
// software and its documentation for any purpose, provided that the
// above copyright notice and the following two paragraphs appear in
// all copies of this software.
//
// IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
// DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
// ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
// IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
// ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
// PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

// https://en.wikipedia.org/wiki/Funnelsort
// https://github.com/sammymax/lazy-funnelsort
// https://github.com/eanikolaev/funnelsort
// https://en.wikipedia.org/wiki/Sorting_algorithm
// https://cglab.ca/~dana/funnelsort/funnelsort.pdf
// https://cs.au.dk/~gerth/papers/alcomft-tr-03-101.pdf
// https://www.ijcseonline.org/spl_pub_paper/ICRIC-2018-025.pdf


#ifndef FUNNELSORT_H
#define FUNNELSORT_H

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>
#include "giopler/giopler.hpp"

using std::vector;
// if size of vector is <= this, don't recurse
// must be at least 8 becase floor(7^ (1/3) ) = 1
const size_t kBaseCase = 8;

namespace {
template <class T, class C>
class FunnelTree {
	public:
		C comp;

		FunnelTree *left = nullptr, *right = nullptr;
		// the output buffer for this node; constbuffer is for leaves
		T *buffer;
		const T *constbuffer;
		size_t start = 0, end, cap;
		// will calling fill() add any new numbers?
		bool can_fill = true;

		T get() {
			return (constbuffer == nullptr) ? buffer[start] : constbuffer[start];
		}

		T selftake() {
			T res = get();
			start++;
			return res;
		}

		T righttake() {
			T res = right->selftake();
			comp.right_take_update(res);
			return res;
		}

		T lefttake() {
			T res = left->selftake();
			comp.left_take_update(res);
			return res;
		}

		void fill();
};

// assumption: this can only be called when start = end
template <class T, class C>
void FunnelTree<T, C>::fill() {
	if (!can_fill) return;
	start = 0;
	end = 0;
	while (end < cap) {
		if (left->start == left->end)
			left->fill();
		if (right->start == right->end)
			right->fill();

		can_fill = left->can_fill || (left->start != left->end) ||
			right->can_fill || (right->start != right->end);
		if (!can_fill) {
			break;
		}
		if (left->start == left->end)
			buffer[end++] = righttake();
		else if (right->start == right->end)
			buffer[end++] = lefttake();
		else {
			T a = left->get(), b = right->get();
			if (comp(a, b)) buffer[end++] = righttake();
			else buffer[end++] = lefttake();
		}
	}
}

void calculate_bufsizes_(vector<size_t>& v, int l, int r, size_t size) {
	// take square root of size
	size = pow(size, 0.5) + 1;
	if (r - l <= 2) {
		for (; l < r; l++) v[l] = size;
		return;
	}
	int m = (l + r) / 2;
	v[m] = size;
	calculate_bufsizes_(v, l, m, size);
	calculate_bufsizes_(v, m + 1, r, size);
}

vector<size_t> calculate_bufsizes(int depth, size_t initial_size) {
	vector<size_t> res(depth);
	res[0] = initial_size;
	calculate_bufsizes_(res, 1, depth, initial_size);
	return res;
}

template <class T, class Comp>
FunnelTree<T, Comp>* create_tree_(int d, const vector<size_t>& depth_to_size,
							const vector<vector<T>>& lists, int l, int r,
							T* buffer, size_t *buf_used, Comp comp) {
	auto *res = new FunnelTree<T, Comp>;
	res->comp = comp;
	// this node represents a single thing; leaf node
	if (r == l + 1) {
		res->constbuffer = lists[l].data();
		res->end = lists[l].size();
		res->cap = lists[l].size();
		res->can_fill = false;
		return res;
	}
	// since it's un-populated
	res->end = 0;
	res->cap = depth_to_size[d];

	res->constbuffer = nullptr;
	res->buffer = buffer + (*buf_used);
	*buf_used += res->cap;
	int m = (l + r) / 2;
	res->left = create_tree_(d+1, depth_to_size, lists, l, m, buffer, buf_used, comp);
	res->right = create_tree_(d+1, depth_to_size, lists, m, r, buffer, buf_used, comp);
	return res;
}

template <class T, class Comp>
FunnelTree<T, Comp>* create_tree(const vector<vector<T>>& lists, Comp comp) {
	size_t k = lists.size(), tot_size = 0;
	for (auto& l : lists)
		tot_size += l.size();

	size_t max_depth = 0;
	while ((2u << max_depth) < k)
		max_depth++;

	const vector<size_t> bufsizes = calculate_bufsizes(max_depth + 1, tot_size);
	size_t tot_buf_size = 0;
	for (size_t i = 0; i < max_depth; i++)
		tot_buf_size += (1<<i) * bufsizes[i];
	// total number of nodes is always 2k-1; subtract (2^max_depth - 1)
	tot_buf_size += (2*k - (1 << max_depth)) * bufsizes[max_depth];
	T* buffer = new T[tot_buf_size];

	size_t buf_used = 0;
	return create_tree_(0, bufsizes, lists, 0, k, buffer, &buf_used, comp);
}
}  // namespace

template <class RandomIt, class Comp>
vector<typename RandomIt::value_type> funnelsort(RandomIt first, RandomIt last, Comp comp);

template <class RandomIt>
class default_sort {
	//class provided to lazy funnelsort for std::sort
	typedef typename RandomIt::value_type T;

	public:
	bool operator() (const T& a, const T& b) {
		return a > b;
	}

	void right_take_update(const T& t) {
		//Update information of point taken from right buffer in merge step
		return;
	}

	void left_take_update(const T& t) {
		//Update information of point taken from left buffer in merge step
		return;
	}

	std::vector<T> base_case(std::vector<T> arr) {
		//base case to take by lazy funnelsort
		std::sort(arr.begin(), arr.end());
		return arr;
	}
};

template <class RandomIt>
vector<typename RandomIt::value_type> funnelsort(RandomIt first, RandomIt last) {
    giopler::dev::Function function;
	return funnelsort(first, last, default_sort<RandomIt>());
	//return funnelsort(first, last, std::greater<typename RandomIt::value_type>());
}

template <class T, class Comp>
vector<T> merge(const vector<vector<T>>& lists, Comp comp);

template <class RandomIt, class Comp>
vector<typename RandomIt::value_type> funnelsort(RandomIt first, RandomIt last, Comp comp) {
	typedef typename RandomIt::value_type T;
	size_t size = last - first;
	if (size <= kBaseCase) {
		vector<T> arr_copy(first, last);
		return comp.base_case(arr_copy);
	}

	int szCubeRoot = pow(size, 1.0/3.0);
	vector<vector<T>> recursive_results(szCubeRoot);
	int loSize = size / szCubeRoot;
	// x * loSize + (szCubeRoot - x) * (loSize + 1) = size
	// szCubeRoot * loSize + szCubeRoot - x = size
	// x = szCubeRoot * (loSize + 1) - size
	int numLoSize = szCubeRoot * (loSize + 1) - size;
	for (int i = 0; i < szCubeRoot; i++) {
		int curSize = (i < numLoSize) ? loSize : loSize + 1;
		recursive_results[i] = funnelsort(first, first + curSize, comp);
		first += curSize;
	}
	return merge(recursive_results, comp);
}

template <class T, class Comp>
vector<T> merge(const vector<vector<T>>& lists, Comp comp) {
	auto root = create_tree(lists, comp);
	root->fill();
	vector<T> res(root->buffer, root->buffer + root->cap);
	return res;
}

#endif  // FUNNELSORT_H


template <class RandomIt>
void stl_sort(RandomIt first, RandomIt last) {
    giopler::dev::Function function;
    sort(first, last);
}

// -----------------------------------------------------------------------------
// Note: As written, this program will send about 8 events to the Giopler servers.
// This number will vary depending on the build mode.
int main()
{
  constexpr size_t elements = 1024;

  std::vector<int> values(elements);
  for (auto&& x : values)   x = rand();

  {
    auto res = funnelsort(values.begin(), values.end());

    bool ok = true;
    for (size_t j = 0; j < elements; j++) {
        if (res[j] != values[j])   ok = false;
    }
    giopler::dev::confirm(ok);
  }

  for (auto&& x : values)   x = rand();
  stl_sort(values.begin(), values.end());
}
