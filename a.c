//change from l4 to l1
//@security (x, -s l4)
int x;

#if 0
////@security (a, -s l1) (b, -a a) (c, -a b) 
#endif

//@security (a, -s l4) (b, -s l4) (c, -s l4) 
void work(int a, int b, int c) {
    x = a;
}

//@security (y, -s l2)
int y;

struct c {
    int a;
    int b;
};

//@security (w, -s l2)
struct c w;

//@security 
//work function starts
int main(){
    x = 1;
    y = 2;
    x = y + y;
    x = x + y;
    x = -x;
    work(x, y, 1);

    while (1) {
        x = 1;
        break;
    }
   
#if 0
    if (x == -6) {
        x = x + 1;
    }
    else {
        x = x - 1;
    }
#endif 
    &x;
    return 0;
}
//work function ends
