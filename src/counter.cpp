#include <iostream>
using namespace std;
int randgen () ;
main() {
//   unsigned int d=0, masked = 0xF0;
// for ( unsigned int i=0 ; i<33; i++){
//   
////   d = i & masked ; //d>>=4;            // 15 = 0000 1111
//////if(d==1)
////   cout << "condition true " << hex <<  i << " " << hex << d << endl ;
//	d= ( randgen() % 31);
//	cout << " the rand value is" << d << endl ;
//
//}

unsigned long long temp;
temp = 16385 ;
unsigned int temp1 = ((unsigned int)temp) & 16383 ;
cout << " out " << temp1 ;


   return 0;
}

int randgen()
{
return rand();
}
