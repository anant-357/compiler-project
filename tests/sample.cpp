#include <bits/stdc++.h>

using namespace std;

int fun1(int x) {
  cout << "printing arg x in fun1 : " << x << "\n";
  return 2;
}

int fun2(int x) {
  cout << "printing arg x in fun2 : " << x << "\n";
  return 3;
}

int main() {
  int c, d;
  cin >> d;
  cin >> c;
  if (c == 2) {
    fun1(d);
  } else {
    for (int i = 0; i < c; i++) {
      fun2(i);
    }
  }
  return 1;
}
