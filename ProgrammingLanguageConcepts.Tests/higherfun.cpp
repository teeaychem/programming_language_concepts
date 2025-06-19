#include <cstdio>
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
    std::string str{};

    Node<T> *curr = first;

    if (curr == nullptr) {
      return str;
    }

    while (true) {
      str.append(std::to_string(curr->item()));
      curr = curr->next();
      if (curr != nullptr) {
        str.push_back(' ');
      } else {
        break;
      }
    }

    return str;
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

// Exercise 5.1
LinkedList<int> merge_sorted_lists(LinkedList<int> const &a,
                                        LinkedList<int> const &b) {

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

void merge_sorted_arrays_test() {
  std::vector<int> xs{3, 5, 12};
  std::vector<int> ys{2, 3, 4, 7};

  std::vector<int> out = merge_sorted_arrays(xs, ys);

  for (auto it = out.begin(); it != out.end(); ++it) {
    printf("%d ", *it);
  }
  printf("\n");
}

void merge_sorted_lists_test() {
  LinkedList<int> a{};
  a.add(3);
  a.add(5);
  a.add(12);

  LinkedList<int> b{};
  b.add(2);
  b.add(3);
  b.add(4);
  b.add(7);

  LinkedList<int> merged = merge_sorted_lists(a, b);

  printf("a: [%s], b: [%s], sorted: [%s]\n", a.to_string().c_str(),
         b.to_string().c_str(), merged.to_string().c_str());
}

int main(int argc, char *argv[]) {

  merge_sorted_arrays_test();
  merge_sorted_lists_test();
}
