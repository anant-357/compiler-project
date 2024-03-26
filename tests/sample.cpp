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
  int c;
  cin >> c;
  if (c == 2) {
    fun1(c);
  } else {
    for (int i = 0; i < 5; i++) {
      fun2(i);
    }
  }
  return 1;
}
