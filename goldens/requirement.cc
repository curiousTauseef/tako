
int main(int argc, char* argv[]) {
  const auto x = [&] (const int x_y) {
    if(!(x_y)) x_y else 
    throw 101;
  };
  return x(0);
}