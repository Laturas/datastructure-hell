#include <bits/stdc++.h>
#define ll long long

using namespace std;

struct Node {
    int l, r, sum;
    int tag;
    Node *lc, *rc;
    Node (int bl, int br) {
        l = bl, r = br, sum = 0, tag = 0;
        int mid = (bl + br) >> 1;
        if (bl==br) lc = rc = nullptr;
        else lc = new Node(bl, mid), rc = new Node(mid + 1, br);
    }

    ~Node() {
        delete lc;
        delete rc;
    }

    void add(int bl, int br, int val) {
        // break condition 
        if (r < bl || br < l) return;
        // tag condition
        if (bl <= l && r <= br) {
            // cout << "dbg";
            putTag(val);
            return;
        }
        int within = min(r, br) - max(l, bl) + 1;
        sum += val * within;
        lc -> add (bl, br, val);
        rc -> add (bl, br, val);
    }

    int qry(int ql, int qr, int tagVal) {
        if (ql > r || qr < l) {
            putTag(tagVal);
            return 0;
        }
        if (ql <= l && qr >= r) {
            putTag(tagVal);
            return sum + tag * (1 + r - l);
        }
        tagVal += getTag();
        sum += tagVal * (r - l + 1);
        int mid = (ql + qr) >> 1;
        return lc -> qry(ql, qr, tagVal) + rc -> qry(ql, qr, tagVal);
    }

    void putTag(int tagVal) {
        if (l == r) sum += tagVal;
        else tag += tagVal;
    }
    
    int getTag() {
        int tmp = tag;
        tag = 0;
        return tmp;
    }

    void print() {
        if (l == r) cout << sum;
        else {
            cout << " (";
            lc -> print();
            cout << "[";
            cout << sum;
            cout << ", ";
            cout << tag;
            cout << "]";
            rc -> print();
            cout << ") ";
        }
    }
};

int main (){
    std::ios_base::sync_with_stdio(false); std::cin.tie(nullptr); 
    std::cout << std::fixed << std::setprecision(10);
}
