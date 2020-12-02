#ifndef FIXED_POINT_OP_H
#define FIXED_POINT_OP_H

typedef long long fp;

#define Convert_n_to_fixed_point(n) (n*(1<<14))
#define Convert_x_to_integer_zero(x) (x/(1<<14))
#define Convert_x_to_integer_nearest(x) x >=0 ? ((x+(1<<14)/2)/(1<<14)):((x-(1<<14)/2)/(1<<14))
#define Add_x_and_y(x,y) (x+y)
#define Substract_y_from_x(x,y) (x-y)
#define Add_x_and_n(x,n) (x+n*(1<<14))
#define Substract_n_from_x(x,n) (x-n*(1<<14))
#define Multiply_x_by_y(x,y) ((fp)x)*y/(1<<14)
#define Multiply_x_by_n(x,n) x*n
#define Divide_x_by_y(x,y) ((fp)x)*(1<<14)/(y)
#define Divide_x_by_n(x,n) x/n

#endif
