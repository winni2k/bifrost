#ifndef MINHASHITERATOR_H
#define MINHASHITERATOR_H

#include <stdint.h>
#include <deque>
#include <vector>

#include <iostream>
using namespace std;

//using std::vector;

struct minHashResult {

	minHashResult() : hash((uint64_t) -1),pos(-1) {}
	minHashResult(const uint64_t h, const int p) : hash(h), pos(p) {}
	minHashResult(const minHashResult& o) : hash(o.hash), pos(o.pos) {}
	uint64_t hash;
	int pos;
};

template<class HF>
struct minHashResultIterator;

// TODO: return current position as well as minimum
// templated?
template<class HF>
class minHashIterator {

    public:

        minHashIterator(int _k, int _g, HF _h) : s(NULL), n(0), k(_k), g(_g), hf(_h), v(k-g+1), p(-1), invalid(true), nh(false) {
            hf.setK(g);
        }

        minHashIterator(const char* _s, int _length, int _k, int _g, HF _h, bool _nh) : k(_k), g(_g), hf(_h), v(k-g+1), p(-1), invalid(true), nh(_nh) {
            hf.setK(g);
            initString(_s,_length);
        }

        minHashIterator() : s(NULL), n(0), k(0), g(0), hf(HF(0)), invalid(true), nh(false) {}

        void seed(HF &ohf) {
            hf = ohf;
            invalid=true;
        }

        void initString(const char* _s, int _length) {

            n = _length;
            s = _s;
            p = -1;// reset position
            invalid = false;
            v.clear();

            // advance to first position or set to invalid
            if (n < k || k < g) invalid = true;
            else operator++();
        }


        bool operator==(const minHashIterator& o) {

            if(invalid || o.invalid) return invalid && o.invalid;
            return s==o.s && n==o.n && g==o.g && k==o.k && nh==o.nh;
        }

        bool operator!=(const minHashIterator& o) { return !this->operator==(o); }

        // invariant:
        //   v[0..sz] contains only ascending elements in s from [p,p+k-g+1)
        //
        //   there exists no a<b s.t. v[a] > v[b].
        minHashIterator& operator++() {
            //cerr << "operator++";
            if (invalid) return *this;

            ++p; // advance to next k-mer

            if (p >= n-k+1 || s[p+k-1] == 0) {
                // out of bounds
                invalid = true;
                return *this;
            }

            const int shift = nh ? 1 : 0;

            if (p==0) {

                hf.init(&s[shift]);
                v.push_back(minHashResult(hf.hash(), shift));

                for (int j = shift; j < k-g-shift;) {

                    hf.update(s[j],s[j+g]);

                    uint64_t h = hf.hash();
                    int t = ((int)v.size())-1;

                    while (t >= 0 && v[t].hash > h) {

                        v.pop_back();
                        t--;
                    }

                    j++;
                    v.push_back(minHashResult(h,j));
                }
            }
            else {

                if (v[0].pos < p + shift) v.pop_front(); // remove first element, fell outside of window

                hf.update(s[p+k-g-1-shift],s[p+k-1-shift]);

                uint64_t h = hf.hash();
                int t = ((int) v.size())-1;

                while (t >= 0 && v[t].hash > h) {

                    v.pop_back();
                    t--;
                }

                v.push_back(minHashResult(h,p+k-g-shift));
            }

            return *this;
        }

        minHashIterator operator++(int) {

            minHashIterator tmp(*this);
            operator++();

            return tmp;
        }

        minHashIterator& operator+=(int i) {

            for (; i > 0; i--) operator++();
            return *this;
        }

        minHashResultIterator<HF> operator*() const {return minHashResultIterator<HF>(this);}
        //minHashResultIterator<HF>* operator->() { return &(operator*());}

        uint64_t getHash() const { return invalid ? 0 : v[0].hash; }

        int getPosition() const { return invalid ? 0 : v[0].pos; }

        const char *s; //Minimizers are from k-mers, k-mers are from a sequence s
        int n; // Length of sequence s
        int k; // Length of k-mers
        int g; // Length of minimizers
        HF hf; // Rolling hash function
        deque<minHashResult> v; //Hash values and positions of a same minimizer with k-mer at position p
        int p; // Position of current k-mer traversed in the sequence
        bool invalid; // If sequence is invalid (iterating on k-mers out of bounds, etc.)
        bool nh;

        // private copy constructor
        minHashIterator(const minHashIterator& o) : s(o.s), n(o.n), k(o.k), g(o.g), hf(o.hf), v(o.v), p(o.p), invalid(o.invalid), nh(o.nh) {}
};

template <class HF>
struct minHashResultIterator {

    minHashResultIterator(const minHashIterator<HF> *p) : p(p), invalid(false), pos(0), p_pos(p->p), p_s(p->s) {}
    minHashResultIterator() : invalid(true), p_pos(-1), p_s(NULL) {}

    bool operator==(const minHashResultIterator& o) {

        if (o.invalid || invalid) return o.invalid && invalid;
        return p_pos == o.p_pos && p_s == o.p_s && pos==o.pos;

    }

    bool operator!=(const minHashResultIterator& o) {
        return !this->operator==(o);
    }

    minHashResultIterator& operator++() {

        if (invalid) return *this;

        if ((p_pos != p->p || p_s != p->s) // check if parent iterator has moved
            || (pos>=p->v.size()-1) // or if we advance past the end
            || (p->v[pos+1].hash != p->v[pos].hash))    { // or if the next position doesn't share the hash value
            // invalidate the iterator
            p_s = NULL;
            invalid = true;
            return *this;
        }
        else pos++; // advance to next equal hash value

        return *this;
    }


    minHashResultIterator operator++(int) {

        minHashResultIterator tmp(*this);
        operator++();

        return tmp;
    }

    const minHashResult& operator*() const { return p->v[pos]; }
    const minHashResult* operator->() const { return &(p->v[pos]); }

    // pos points to a minHashIterator, all the values from p.v[0] to p.v[pos] have the
    // same (minimum) hash value or the this.invalid is true
    // at the time this was created p.s==p_s and p.p=p_pos
    const minHashIterator<HF> *p;
    bool invalid;
    int pos;
    const int p_pos;
    const char *p_s;
};






template <class HF>
struct minHashKmer {

    public:

        minHashKmer(const char* _s, int _k, int _g, HF _h, bool neighbor_hash) : s(_s), k(_k), g(_g), hf(_h), h(0), p(-1), invalid(true), nh(neighbor_hash) {

            if ((s != NULL) && ((n = strlen(s)) >= k) && (k >= g)){

                invalid = false;

                hf.setK(g);
                compute_min();
            }
        }

        minHashKmer() : s(NULL), n(0), k(0), g(0), hf(HF(0)), h(0), p(-1), invalid(true), nh(false) {}

        bool operator==(const minHashKmer& o) {

            if(invalid || o.invalid) return invalid && o.invalid;
            return s==o.s && n==o.n && g==o.g && k==o.k && nh==o.nh && h==o.h && p==o.p;
        }

        bool operator!=(const minHashKmer& o) { return !this->operator==(o); }

        uint64_t getHash() const { return h; }

        int getPosition() const { return p; }

    private:

        void compute_min(){

            if (invalid) return;

            const int shift = nh ? 1 : 0;

            hf.init(&s[shift]);

            h = hf.hash();
            p = shift;

            for (int j = shift; j < k-g-shift; j++) {

                hf.update(s[j], s[j+g]);

                if (hf.hash() < h){

                    h = hf.hash();
                    p = j + 1;
                }
            }
        }

        const char* s;
        HF hf;
        uint64_t h;
        int n;
        int k;
        int g;
        int p;
        bool invalid;
        bool nh;
};







template<class HF>
struct preAllocMinHashResultIterator;

template<class HF>
class preAllocMinHashIterator {

    public:

        preAllocMinHashIterator() : s(NULL), n(0), k(0), g(0), hf(HF(0)), p(-1), v(0), p_cur_start(0), p_cur_end(0), invalid(true), nh(false) {}

        preAllocMinHashIterator(const char* _s, int _n, int _k, int _g, HF _h, bool _nh) :
                                s(_s), n(_n), k(_k), g(_g), hf(_h), p(-1), p_cur_start(0), p_cur_end(0), invalid(true), nh(_nh) {

            if ((s != NULL) && (n >= k) && (k >= g)){

                invalid = false;

                v = vector<minHashResult>(n-g+1);

                hf.setK(g);
                operator++();
            }
        }

        bool operator==(const preAllocMinHashIterator& o) {

            if (invalid || o.invalid) return invalid && o.invalid;
            return s==o.s && n==o.n && g==o.g && k==o.k && nh==o.nh;
        }

        bool operator!=(const preAllocMinHashIterator& o) { return !this->operator==(o); }

        // invariant:
        //   v[0..sz] contains only ascending elements in s from [p,p+k-g+1)
        //
        //   there exists no a<b s.t. v[a] > v[b].
        preAllocMinHashIterator& operator++() {
            //cerr << "operator++";
            if (invalid) return *this;

            ++p; // advance to next k-mer

            if (p >= n-k+1 || s[p+k-1] == 0) {
                // out of bounds
                invalid = true;
                return *this;
            }

            const int shift = nh ? 1 : 0;

            if (p == 0) {

                hf.init(&s[shift]);

                v[p_cur_end] = minHashResult(hf.hash(), shift);
                p_cur_end++;

                for (int j = shift; j < k-g-shift;) {

                    hf.update(s[j], s[j+g]);

                    uint64_t h = hf.hash();

                    while (p_cur_end > p_cur_start && v[p_cur_end-1].hash > h) p_cur_end--;

                    j++;

                    v[p_cur_end] = minHashResult(h,j);
                    p_cur_end++;
                }
            }
            else {

                if (v[p_cur_start].pos < p + shift) p_cur_start++; // remove first element, fell outside of window

                hf.update(s[p+k-g-1-shift], s[p+k-1-shift]);

                uint64_t h = hf.hash();

                while (p_cur_end > p_cur_start && v[p_cur_end-1].hash > h) p_cur_end--;

                v[p_cur_end] = minHashResult(h,p+k-g-shift);
                p_cur_end++;
            }

            return *this;
        }

        preAllocMinHashIterator operator++(int) {

            preAllocMinHashIterator tmp(*this);
            operator++();

            return tmp;
        }

        preAllocMinHashIterator& operator+=(int i) {

            for (; i > 0; i--) operator++();
            return *this;
        }

        preAllocMinHashResultIterator<HF> operator*() const {return preAllocMinHashResultIterator<HF>(this);}

        uint64_t getHash() const { return invalid ? 0 : v[p_cur_start].hash; }

        int getPosition() const { return invalid ? 0 : v[p_cur_start].pos; }

        const char *s; //Minimizers are from k-mers, k-mers are from a sequence s
        int n; // Length of sequence s
        int k; // Length of k-mers
        int g; // Length of minimizers
        HF hf; // Rolling hash function
        vector<minHashResult> v; //Hash values and positions of a same minimizer with k-mer at position p
        size_t p_cur_start;
        size_t p_cur_end;
        int p; // Position of current k-mer traversed in the sequence
        bool invalid; // If sequence is invalid (iterating on k-mers out of bounds, etc.)
        bool nh;

        // private copy constructor
        preAllocMinHashIterator(const preAllocMinHashIterator& o) : s(o.s), n(o.n), k(o.k), g(o.g), hf(o.hf), v(o.v), p(o.p), invalid(o.invalid), nh(o.nh) {}
};

template <class HF>
struct preAllocMinHashResultIterator {

    preAllocMinHashResultIterator(const preAllocMinHashIterator<HF> *p) : p(p), p_pos(p->p), p_s(p->s), p_it(p->p_cur_start), invalid(false) {}
    preAllocMinHashResultIterator() : invalid(true), p_pos(-1), p_s(NULL), p_it(0) {}

    bool operator==(const preAllocMinHashResultIterator& o) {

        if (o.invalid || invalid) return o.invalid && invalid;
        return p_pos == o.p_pos && p_s == o.p_s && p_it==o.p_it;

    }

    bool operator!=(const preAllocMinHashResultIterator& o) {
        return !this->operator==(o);
    }

    preAllocMinHashResultIterator& operator++() {

        if (invalid) return *this;

        if ((p_s != p->s || p_pos != p->p) // check if parent iterator has moved
            || (p_it >= p->p_cur_end) // or if we advance past the end
            || (p->v[p_it + 1].hash != p->v[p_it].hash)) { // or if the next position doesn't share the hash value
            // invalidate the iterator
            p_s = NULL;
            invalid = true;
            return *this;
        }
        else p_it++; // advance to next equal hash value

        return *this;
    }


    preAllocMinHashResultIterator operator++(int) {

        preAllocMinHashResultIterator tmp(*this);
        operator++();

        return tmp;
    }

    const minHashResult& operator*() const { return p->v[p_it]; }
    const minHashResult* operator->() const { return &(p->v[p_it]); }

    // pos points to a minHashIterator, all the values from p.v[0] to p.v[pos] have the
    // same (minimum) hash value or the this.invalid is true
    // at the time this was created p.s==p_s and p.p=p_pos
    const preAllocMinHashIterator<HF> *p;
    bool invalid;
    size_t p_it;
    const int p_pos;
    const char *p_s;
};

#endif // MINHASHITERATOR_H