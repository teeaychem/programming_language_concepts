#include <cstdio>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

template <typename T>
class Node {
public:
  T _item;
  Node *_next;

  Node(T item) : _item(item) {};

public:
  T item() const { return _item; }
  Node *next() const { return _next; }
  void set_next(Node *to) { _next = to; }
};

template <typename T>
class LinkedList {

  Node<T> *first = nullptr;
  Node<T> *last = nullptr;

public:
  Node<T> *head() const { return first; }

  void add(T item) {
    if (last == nullptr) {
      last = new Node<T>(item);
      first = last;
    } else {
      Node<T> *tmp = new Node<T>(item);
      last->set_next(tmp);
      last = tmp;
      if (first == nullptr) {
        first = tmp;
      }
    }
  }

  std::string to_string() {
    std::stringstream ss{};

    Node<T> *curr = first;

    if (curr == nullptr) {
      return ss.str();
    }

    while (true) {
      ss << curr->item();
      curr = curr->next();
      if (curr != nullptr) {
        ss << ' ';
      } else {
        break;
      }
    }

    return ss.str();
  }
};

// Exercise 5.1
std::vector<int> merge_sorted_arrays(std::vector<int> const &a,
                                     std::vector<int> const &b) {

  int a_idx = 0;
  int b_idx = 0;

  std::vector<int> out{};
  out.reserve(a.size() + b.size());

  while (a_idx < a.size() && b_idx < b.size()) {
    if (a[a_idx] < b[b_idx]) {
      out.push_back(a[a_idx++]);
    } else {
      out.push_back(b[b_idx++]);
    }
  }

  while (a_idx < a.size()) {
    out.push_back(a[a_idx++]);
  }

  while (b_idx < b.size()) {
    out.push_back(b[b_idx++]);
  }

  return out;
}

// Exercise 5.2
LinkedList<int> merge(LinkedList<int> const &a, LinkedList<int> const &b) {

  Node<int> *ah = a.head();
  Node<int> *bh = b.head();

  LinkedList<int> out{};

  while (ah != nullptr && bh != nullptr) {
    if (ah->item() < bh->item()) {
      out.add(ah->item());
      ah = ah->next();
    } else {
      out.add(bh->item());
      bh = bh->next();
    }
  }

  while (ah != nullptr) {
    out.add(ah->item());
    ah = ah->next();
  }

  while (bh != nullptr) {
    out.add(bh->item());
    bh = bh->next();
  }

  return out;
}

template <typename T>
LinkedList<T> mergep(LinkedList<T> const &a, LinkedList<T> const &b) {

  Node<T> *ah = a.head();
  Node<T> *bh = b.head();

  LinkedList<T> out{};

  while (ah != nullptr && bh != nullptr) {
    if (ah->item() < bh->item()) {
      out.add(ah->item());
      ah = ah->next();
    } else {
      out.add(bh->item());
      bh = bh->next();
    }
  }

  while (ah != nullptr) {
    out.add(ah->item());
    ah = ah->next();
  }

  while (bh != nullptr) {
    out.add(bh->item());
    bh = bh->next();
  }

  return out;
}

// Exercise 5.3
template <typename T>
LinkedList<T> mergec(LinkedList<T> const &a, LinkedList<T> const &b,
                     std::function<int(T, T)> cmp) {

  Node<T> *ah = a.head();
  Node<T> *bh = b.head();

  LinkedList<T> out{};

  while (ah != nullptr && bh != nullptr) {
    if (cmp(ah->item(), bh->item()) < 1) {
      out.add(ah->item());
      ah = ah->next();
    } else {
      out.add(bh->item());
      bh = bh->next();
    }
  }

  while (ah != nullptr) {
    out.add(ah->item());
    ah = ah->next();
  }

  while (bh != nullptr) {
    out.add(bh->item());
    bh = bh->next();
  }

  return out;
}

void five_point_one() {
  std::vector<int> xs{3, 5, 12};
  std::vector<int> ys{2, 3, 4, 7};

  std::vector<int> out = merge_sorted_arrays(xs, ys);

  for (auto it = out.begin(); it != out.end(); ++it) {
    printf("%d ", *it);
  }
  printf("\n");
}

void five_point_two() {
  LinkedList<int> a{};
  a.add(3);
  a.add(5);
  a.add(12);

  LinkedList<int> b{};
  b.add(2);
  b.add(3);
  b.add(4);
  b.add(7);

  LinkedList<int> merged = merge(a, b);

  printf("a: [%s], b: [%s], sorted: [%s]\n", a.to_string().c_str(),
         b.to_string().c_str(), merged.to_string().c_str());
}

void five_point_three_b() {

  LinkedList<std::string> a{};

  a.add("abc");
  a.add("apricot");
  a.add("ballad");
  a.add("zebra");

  LinkedList<std::string> b{};
  b.add("abelian");
  b.add("ape");
  b.add("carbon");
  b.add("yosemite");

  LinkedList<std::string> mergedp = mergep(a, b);

  printf("a: [%s], b: [%s], sorted: [%s]\n", a.to_string().c_str(),
         b.to_string().c_str(), mergedp.to_string().c_str());
}

void five_point_three_c() {
  LinkedList<int> a{};
  a.add(3);
  a.add(5);
  a.add(12);

  LinkedList<int> b{};
  b.add(2);
  b.add(3);
  b.add(4);
  b.add(7);

  std::function<int(int, int)> icmp = [](int x, int y) {
    return x < y ? -1 : (x > y ? 1 : 0);
  };

  LinkedList<int> mergedc = mergec(a, b, icmp);

  printf("a: [%s], b: [%s], sorted: [%s]\n", a.to_string().c_str(),
         b.to_string().c_str(), mergedc.to_string().c_str());
}

int main(int argc, char *argv[]) {

  five_point_one();
  five_point_two();
  five_point_three_b();
  five_point_three_c();
}
