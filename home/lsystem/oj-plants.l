; l-systems from Ole Jorgensen 

Sporrer { ; 
  angle 4
  axiom F+F+F+F
  F=-F+F
} 

Sporrer2 { ; 
  angle 4
  axiom F+F+F+F
  F=+<1F-<1F
} 

1b { ; 
  Angle 15   
  Axiom X
  X=-<2F-<2F[-X]-<2F[-X]+F>2[-X]+F>2[-X]+F>2[-X]
  F=<5F<5F>5F>5
}

1bc { ; 
  Angle 15   
  Axiom X
  X=-<2F[+X][-X]-<2F[+X][-X]-<2F[+X][-X]+F>2[+X][-X]+F>2[+X][-X]+F>2[+X][-X]
  F=<2F@.9985F>2@.9945F
}

1bc_2 { ;
  Angle 15   
  Axiom x
  X=<5F<5FF[s][t]<5FF[s][t]FF>5[s][t]@.9985FF>5[s][t]@.9945FF>5[s][t]
  s=+<3F+<4F[+s][-t]+<5F[+s][-t]+F[+s][-t]
  s=-F>5[+s][-t]-F>4[+s][-t]-F>3[+s][-t]
  t=-<3F-<4F[+s][-t]-<5F[+s][-t]-F[+s][-t]
  t=+F>5[+s][-t]+F>4[+s][-t]+F>3[+s][-t]
  F=<10F<10F>10@.9985F>10 
}

1bd { ; 
  Angle 30   
  Axiom +++X
  X=-<3F-<4F[+X][-X]-<5F[+X][-X]-F>5[+X][-X]+F>4[+X][-X]+F>3[+X][-X]
  F=<5F<5F>5F>5
}

1bd_2 { ; 
  Angle 30   
  Axiom +++x
  X=<3F<4F[s][t]<5F[s][t]F>5[s][t]F>4[s][t]F>3[s][t]
  s=+<3F+<4F[+s][-t]+<5F[+s][-t]
  s=+F>5[+s][-t]-F>4[+s][-t]-F>3[+s][-t]
  t=-<3F-<4F[+s][-t]-<5F[+s][-t]
  t=-F>5[+s][-t]+F>4[+s][-t]+F>3[+s][-t]
  F=<10F<10F>10@.9985F>10 
}

1be { ; 
  Angle 30   
  Axiom X
  X=-<2F[+X][-X]-<2@0.95F[+X][-X]-<2@0.9F[+X][-X]-@0.85F>2[+X][-X]-@0.8F>2[+X][-X]-@0.75F>2[X]
  F=<5F<5F>5F>5
}

1be_2 { ; 
  Angle 30   
  Axiom X
  X=<2F[s][t]<2@0.95F[s][t]<2@0.9F[s][t]@0.85F>2[s][t];@0.8F>2[s][t]F>2[s][t]
  ;s=-<2F[+++s][-s]-<2@0.95F[+++s][-s]-<2@0.9F[+++s][-s]
  ;s=-@0.85F>2[+++s][-s]-@0.8F>2[+++s][-s]-@0.75F>2[s]
  ;t=+<2F[+t][---t]+<2@0.95F[+t][---t]+<2@0.9F[+t][---t]
  ;t=+@0.85F>2[+t][---t]+@0.8F>2[+t][---t]+@0.75F>2[t]
  ;F=<5F<5F>5F>5  
  ;or ?
  s=-<2F[--s][+t]-<2@0.95F[--s][+t]-<2@0.9F[--s][+t]
  s=-@0.85F>2[-s][t]-@0.8F>2[--s][+t]-@0.75F>2[s]
  t=+<2F[-s][++t]+<2@0.95F[-s][++t]+<2@0.9F[-s][++t]
  t=+@0.85F>2[s][+t]+@0.8F>2[+s][++t]+@0.75F>2[t]
  F=<5F<5F>5F>5  
}

1c { ; 
  Angle 15   
  Axiom X
  X=-<5F[+X][-X]-<5F[+X][-X]+<5F[+X][-X]+F>5[+X][-X]-F>5[+X][-X]-F>5[X]
  F=FFFF
}

1c_2 { ; 
  Angle 15   
  Axiom X
  X=@.9326F[s][t]@.8557F[s][t]@.7788F[s][t]@.7019F[s][t]@.625F[s][t]
  s=-<5F[s][t]-<5F[s][t]+<5F[s][t]
  s=+F>5[s][t]-F>5[s][t]-F>5[s][t]
  t=+<5F[s][t]+<5F[s][t]-<5F[s][t]
  t=-F>5[s][t]+F>5[s][t]+F>5[s][t]
  F=<10FFFFF>10
}

1d { ; 
  Angle 15   
  Axiom x
  ;x=-<5F<5F[+++X][-X]-<5F[+++X][-X]+F>5[+++X][-X]-F>5[+++X][-X]+F>5[+++X][-X]
  x=-<5F+<5F-F[+++X][-X]+F>5-F>5[+++X][-X]
  f=FFF
}

1d_2 { ; 
  Angle 15   
  Axiom x
  x=F[s][t]
  s=-<5F[s][t]+<5F[s][t]-F[s][t]
  s=+F>5[s][t]-F>5[s][t]+F[s][t]  
  t=+<5F[s][t]-<5F[s][t]+F[s][t]
  t=-F>5[s][t]+F>5[s][t]-F[s][t]  
  f=FFFF
}

1e { ; 
  Angle 30  
  Axiom +++X
  X=-<1F-<1F[+X][-X]-<1F[+X][-X]-F>3[+X][-X];-F[+X];-F[+X]
  F=<5F<5F>5F>5
}

1e_2 { ; 
  Angle 45  
  Axiom +++X
  X=@.9F[s][t]@.85F[s][t]@.8F[s][t]@.75F[s][t]@.65F[s][t]
  s=+<5@.95F+<5@.9F[+s][-t]+@.85F>5[+s][-t]+@.8F>5[s]
  t=-<5@.95F-<5@.9F[+s][-t]-@.85F>5[+s][-t]-@.8F>5[t]
  ;?
  ;s=+++<5@.95F[+s][t]+<5@.9F[+s][t]+@.85F>5[+s][t]+@.8F>5[--s]
  ;t=---<5@.95F[+s][t]-<5@.9F[+s][t]-@.85F>5[+s][t]-@.8F>5[+t]
  F=<10ff>10
  ;F=<10fff>10
}

1i { ; 
  Angle 15   
  Axiom +++++X
  X=-<1F[-X]+<3F[-X]-<5F[+X]+F>1[+X]+F>3[-X]-F>5[-X]
  F=<10FFF>10
}

1j { ; 
  Angle 15   
  Axiom |X
  x=F[s][t]
  s=-<1F[s][t]+<3F[s][t]+<5F[s][t]
  s=-F>5[s][t]+F>3[s][t]-F>1[s][t]
  t=+<1F[s][t]-<3F[s][t]-<5F[s][t]
  t=+F>5[s][t]-F>3[s][t]+F>1[s][t]
  F=<10FFF>10
}

2e { ; 
  Angle 15     
  Axiom x
  x=<5F[++Y][--Y]<5@.9326F[++Y][--Y]<5@.8557F[++Y][--Y]
  x=@.7788F>5[++Y][--Y]@.7019F>5[+Y][-Y]@.625F>5[Y]
  y=<5F[++X][--X]<5@.9326F[++X][--X]<5@.8557F[++X][--X]
  y=@.7788F>5[++X][--X]@.7019F>5[+X][-X]@.625F>5[X]
  F=<10F<10F>10F>10
}

2f_2 { ; 
  Angle 30     ; 
  Axiom x
  x=<5@.9326F[s][t]<5@.8557F[s][t]<5@.7788F>5[s][t]
  x=@.7619F>5[s][t]@.725F>5[s][t]
  t=+<1F[++s][t]+<3@.9326F[++s][t]+<5@.7788F[++s][t]
  t=-@.7619F>5[++s][t]-@.725F>3[+s][t]-@.7019F>1[t]
  s=-<1F[s][--t]-<3@.9326F[s][--t]-<5@.7788F[s][--t]
  s=+@.7619F>5[s][--t]+@.725F>3[s][-t]+@.7019F>1[s]
  f=<8FFF>8
}

2g { ; 
  Angle 30     ; 
  Axiom ++++++x
  x=+<5F[---X]+<5F[++X][---X]-<5F[++X][---X]
  x=-F>5[++X][---X]+F>5[++X][---X]+F>5[++X][---X]
  f=<5F<5FF>5F>5
}

Bronchia { ; 
  Angle 30
  Axiom X
  F=--<5F[X][----X]++F>5[X][----X]
  X=++<5F[++++F][F]--F>5[++++F][F]
} 

Bronchia2 { ; 
  Angle 30
  Axiom X
  F=-<1F>1[-X]+<1F>1[+X]
  X=[+F]-<1F>1[+F]-<1F>1
} 

Bronchius { ; 
  Angle 15
  Axiom x
  x=<1F>1+<1F>1[+X][-X]<1F>1-<1F>1[+X][-X] 
  f=<1F>1[+X][-X]<1F>1[+X][-X] 
}

Cirkles { ; 
  Angle 15   
  Axiom |X
  X=F[s][t];@.9F[s][t]@.85F[s][t]@.8F[s][t]@.75F[s][t]@.7F[s][t]@.65F[s][t]
  s=----<1F[s][t]-<5F[s][t]-<10F[s][t]-<10F[s][t]
  s=-F>10[s][t]-F>10[s][t]-F>5[s][t]-F>1[s][t]
  t=++++<1F[s][t]+<5F[s][t]+<10F[s][t]+<10F[s][t]
  t=+F>10[s][t]+F>10[s][t]+F>5[s][t]+F>1[s][t]
  F=<10F<10FFF>10F>10
}

CirkularTree { ; 
  Angle 10
  Axiom F
  F=F[+s]F[-t]
  S=+<2F-<2F-<2F+<2F+<2F+<2F-<2F-<2F-<2F
  S=-<2F-F>2+F>2+F>2+F>2+F>2+F>2+F>2+F>2+F>2
  T=-<2F+<2F+<2F-<2F-<2F-<2F+<2F+<2F+<2F
  T=+<2F+F>2-F>2-F>2-F>2-F>2-F>2-F>2-F>2-F>2
}

Feather { ; 
  Angle 30
  Axiom F
  X=-<10F[-X]-F>10[-X] 
  F=+<10F[-X]-F>10[-X]
}

Feather2 { ;  
  Angle 15 
  Axiom |F
  F=-<10F[+X]+F>10[-S] 
  X=+<1F+<1F-<1F+<1F-<1F+<1F-<1F+<1F-F>1+F>1+F>1-F>1+F>1-F>1+F>1-F>1+F>1
  S=+<1F+<1F-<1F+<1F-<1F+<1F-<1F+<1F-F>1+F>1+F>1-F>1+F>1-F>1+F>1-F>1+F>1
}

Fern { ; Top
  Angle 50      
  Axiom x
  x=<5F[+++++X][-----X]<5F[++++X][----X]<5F[+++X][---X]<5F[++X][--X]
  x=F>5[+X][-X]F>5[X]
  f=<20F<20F>20F>20
}

FernSideView { ; 
  Angle 50      
  Axiom x
  x=-<5F[+++++X][-----X]-<5F[++++X][----X]-<5F[+++X][---X]-<5F[++X][--X]
  x=-F>5[+X][-X]-F>5[X]
  f=<20F<20F>20F>20
}

Fern2 { ;  
  Angle 30    
  Axiom X
  x=<5@.9326F[+l][-r]<5@.8557F[+l][-r]<5@.7788F>5[+l][-r]
  x=@.7619F>5[+l][-r]@.725F>5[l][r]Fx
  l=+<5F[+l][---l]+<5@.9326F[+l][---l]+<5@.9026F[+l][---l]
  l=+@.9326F>5[+l][---l]+@.8557F>5[l][--l]+@.7788F>5[-l]
  r=-<5F[+++r][-r]-<5@.9326F[+++r][-r]-<5@.9326F[+++r][-r]
  r=-@.9326F>5[+++r][-r]-@.8557F>5[++r][r]-@.7788F>5[+r]
  ;or ?
  ;  l=+<5F[+l][r]+<5@.9326F[+l][r]+<5@.8557F[+l][r]
  ;  l=+@.7788F>5[+l][r]+@.7019F>5[+l][r]+@.625F>5[l][+r]
  ;  r=-<5F[l][-r]-<5@.9326F[l][-r]-<5@.8557F[l][-r]
  ;  r=-@.7728F>5[l][-r]-@.7019F>5[l][-r]-@.625F>5[-l][r]
  f=<20F<20F>20F>20
}

Flower5 { ; 
  Angle 30
  Axiom FC1+F+F+F+F+F+FC51+F+F+F+F+F+FC102+F+F+F+F+F+FC68+F+F+F+F+F+FC34+F+F+F+F+F
  F=F[X]
  X=FSFT;FFSSFFFTTT
  S=-FC1[+X]-FC128[-X]
  T=+FC196[-X]+FC64[+X]
}

Grass { ;
  Angle 15
  Axiom x
  x=>5F[-X][+X]>5@0.75F[-X][+X]>5@0.75F[-X][+X]
  x=<5@0.75F[-X][+X]<5@0.75F[-X][+X]<5@0.75F[X]
  f=ff@0.9985F
}

Grass6 { ; 
  Angle 15   
  Axiom +++x
  x=+<1F[+X]-<5F[-X]+<10F[+X]-F>1[+X]+F>5[-X]-F>10[+X]
  f=<10F<10F>10F>10
}

Grass2 { ;  
  Angle 30
  Axiom +++++++F
  F=F[+A]F[--A] 
  A=+c1F-<3FFFFF+<3F-<3FFF+<3F
  A=<3F-<3FF+<3FFF-<3F+<3FFFFF
}

Grass3 { ;
  Angle 15    
  Axiom +++++++x
  x=<5F<5F[-X]+<5F[+X]-F>5[-X]+F>5[+X]-F>5[-X]
  f=<5f<5FF>5F>5
}

Orchid { ;  
  Angle 15
  Axiom F+F+F+F
  D=42D!\84D!\42D
  F=D![+A]D[+B] 
  A=FC1F<21@0.9F<21@0.9F<21@0.9F<21@0.9F<21
  A=@0.9F<21@0.9F<21@0.9F<21@0.9F<21@0.9F<21@0.9F<21
  B=FC255F>21@0.9F>21@0.9F>21@0.9F>21@0.9F>21
  B=@0.9F>21@0.9F>21@0.9F>21@0.9F>21@0.9F>21@0.9F>21
}

Orchid2 { ; 
  Angle 4
  Axiom F+F+F+F+F+F+F+F  
  D=42D!\84D!\42D
  F=D[+A]D[+B] 
  A=+FC1-F<21+F<21F<21-F<21F<21F<21+F<21F<21F<21F<21F<21
  B=-FC255+F>21-F>21F>21+F>21F>21F>21-F>21F>21F>21F>21F>21
}

Leaves { ;
  Angle 15   
  Axiom |x
  x=+<1F[+X][-X]+<5F[+X][-X]+<10F[+X][-X]
  x=-F>1[+X][-X]-F>5[+X][-X]-F>10[+X][-X]
  f=<10FFF>10
}

Leaves2 { ;
  Angle 15   
  Axiom |x
  x=F[++s][--t]@.9326F[+s][-t]@.8557F[s][t]@.7788F[-s]
  s=+<1F[s][t]<5@.9326F[s][t]<10@.8557F[s][t]
  s=@.7788F>1[s][t]@.7019F>5[s][t]@.625F>10[s][t]@.625F[-s]
  t=-<1F[s][t]<5@.9326F[s][t]<10@.8557F[s][t]
  t=@.7788F>1[s][t]@.7019F>5[s][t]@.625F>10[s][t]@.625F[-s]
  f=<10FFF>10
  ;f=<10FF>10
}

Leaves3 { ; 
  Angle 30      
  Axiom x
  x=+<5F[++X][---X]-<5F[++X][---X]-<5@.9326F[++X][---X]
  x=-@.8557F>5[++X][---X]-@.7788F>5[+X][--X]-@.7019F>5[X]
  f=<10F<10F>10F>10
}

Leaves4 { ; 
  Angle 30      
  Axiom x
  x=F[++++s][----t]@.9326F[+++s][---t]
  x=@.8557F[++s][--t]@.7788F[+s][-t];@.7019F[x]
  s=+<5F[++s][---s]-<5F[++s][---s]-<5@.9326F[++s][---s]
  s=-@.8557F>5[++s][---s]-@.7788F>5[+s][--s]-@.7019F>5[s][-s]
  t=-<5F[+++t][--t]+<5F[+++t][--t]+<5@.9326F[+++t][--t]
  t=+@.8557F>5[+++t][--t]+@.7788F>5[++t][-t]+@.7019F>5[+t][t]
  f=<10F<10F>10F>10
  ;f=<10FFFF>10
}

Plant01 { ;
  Angle 15     
  Axiom x
  x=<5F[++x][--x]<5@1.1F[++x][--x]
  x=@.9F>5[+x][-x]@.9F>5[x]
  F=<10FFF>10
}

Plant02 { ;
  Angle 50
  Axiom x
  x=F[s][t]|F[s][t]
  s=-<2F-<2F-<2FF-<2FFF[s][t]-<2FFFFF>2[s][t]
  s=-FFF[s][t]>2-FF[s][t]>2-F[s][t]>2-F>2
  t=+<2F+<2F+<2FF+<2FFF[s][t]+<2FFFFF>2[s][t]
  t=+FFF[s][t]>2+FF[s][t]>2+F[s][t]>2+F[s][t]>2
  f=fffff
}

Plant02B { ;
  Angle 50
  Axiom x
  x=@.95F[s][t]
  s=-<2F-<2F-<2FF-<2FFF[s][t]
  s=-<2F[s][t]F[s][t]F[s][t]F[s][t]F>2[s][t]
  s=-@.95F[s][t]@.95F[s][t]@.95F[s][t]>2
  s=-@.95F[s][t]@.95F[s][t]>2-@.95F[s][t]
  t=+<2F+<2F+<2FF+<2FFF[s][t]
  t=+<2F[s][t]F[s][t]F[s][t]F[s][t]F>2[s][t]
  t=+@.95F[s][t]@.95F[s][t]@.95F[s][t]>2
  t=+@.95F[s][t]@.95F[s][t]>2+@.95F[s][t]
  f=fffff
}

Plant03 { ;
  Angle 15
  Axiom ++FF
  F=+<5F[+F]-<5F[-F+F]F>10
}

Plant03_b { ;
  Angle 15
  Axiom ++FF
  F=+<5F[+F]-F>5[-F+F];F
}

PlantsByRoad { ;
  Angle 15
  Axiom x
  x=+<18F[+x][-x]-F>18[+x][-x]
  f=<10ff>10
}

Plant05 { ;
  Angle 15   
  Axiom +++++X
  X=-<5F[-X]+<10F[+X]-F>5[-X]-F>10[-X]
  F=<5F<10F>5F>10
}

Plant06 { ; 
  Angle 23
  Axiom x
  x=+<5F[--x]--F>5[-x]
  f=-<5F[--f]+F>5[-f] 
}

Plant07 { ; 
  Angle 23
  Axiom X
  X=<5F[+X]<5F[-X]F>5[+X]F>5[-X]
  F=<10FF>10 
}

Plant08 { ; 
  Angle 10
  Axiom F
  F=F[+X]F[-Y]
  X=S-T
  Y=T+S
  S=-<2F+<2F-<2FF+<2FF-<2FFF+<2FFF-FFF>2-FFFFF>2+FFFFF>2-FFFFF>2+FFFFF>2-FFFFF>2
  T=+<2F-<2F+<2FF-<2FF+<2FFF-<2FFF+FFF>2+FFFFF>2-FFFFF>2+FFFFF>2-FFFFF>2+FFFFF>2
}

Plant09 { ; 
  ;
  Angle 30
  Axiom F+F+F
  F=F[X]
  X=FSFT
  S=+F[+X]-F[-X]
  T=-F[-X]+F[+X]
}

Plant10 { ;
  Angle 15     
  Axiom |----f
  f=ff[s][t]f[s][t]
  s=-<5fff[++s][-t]<5f[++s][-t]-<5f[++s][-t]
  s=f>5[++s][-t]f>5[++s][-t]-f>5[++s][-s]
  t=+<5fff[++s][-t]<5f[++s][-t]+<5f[++s][-t]
  t=f>5[++s][-t]f>5[++s][-t]+f>5[++s][-t]
}

Root { ; 
  Angle 23
  Axiom X
  X=+F[-X]--F[+X]
  F=-F[+F]+F[-F] 
}

Sapin { ; 
  Angle 4
  Axiom F+F+F+F  
  D=42D!\84D!\42D
  F=D![+A]D![+B] 
  A=+Fc1-F<21+F<21F<21-F<21F<21F<21+F<21F<21F<21F<21F<21
  B=-Fc255+F>21-F>21F>21+F>21F>21F>21-F>21F>21F>21F>21F>21
}

Sapin2 { ;
  Angle 15  
  Axiom X
  X=>5F[+X][-X]>5F[+X][-X]>5F[+X][-X]>5F<5[+X][-X]F<5[+X][-X]F<5[X]
  F=f@0.9975f@0.9935F
}

Sapin3 { ;
  Angle 15  
  Axiom x
  x=>5F[-X][+X]>5F[--X][++X]>5F<5[---X][+++X]
  x=F<5[----X][++++X]F<5[-----X][+++++X]
  f=c52f@0.9975c32f@0.9935c11F
}

Seaweed { ; 
  Angle 15
  Axiom f
  x=-<5F[+X][-X]+F>5[+X][-X] 
  f=<5F[+X][-X]F>5
}

Seaweed02 { ; 
  Angle 15
  Axiom f
  x=--<5F[X]++F>5[X] 
  y=++<5F[y]--F>5[y]  
  f=<10F[X][y]F>10
}

Seaweed03 { ; 
  Angle 30
  Axiom f
  x=--<5Fy[X]++Fy>5[X] 
  y=++<5Fx[y]--Fx>5[y]  
  f=<10F[X][y]F>10
}

Seaweed04 { ; 
  Angle 15
  Axiom f
  x=--<5F[X][y]++F>5[X][y] 
  y=++<5F[X][y]--F>5[X][y]  
  f=<10F[X][y]F>10
}

Seaweed05 { ; 
  Angle 30
  Axiom f
  x=--<5F[X][y]++F>5[X][y] 
  y=++<5F[X][y]--F>5[X][y]  
  f=<10F[X][y]F>10
}

Seaweed06 { ; 
  Angle 22
  Axiom F
  X=-F[-X]-F[+X] 
  F=F[-X]F
}

Seaweed07 {  ;
  Angle 11
  Axiom F
  F=-<10F[-X]+FF>10[+X]
  X=<10F[+F]<10F[-F]F>10[+F]F>10[-F] 
}

Seaweed08 { ; 
  Angle 15
  Axiom xy
  x=<1+F>1+<1F>1[+y][-y]<1F>1-<1F>1[+y][-y] 
  y=<1-F>1-<1F>1[+x][-x]<1F>1+<1F>1[+x][-x] 
  f=<10F[+x][-x][+y][-y]F>10
}

Seed {  ;
  axiom st
  angle 50
  s=<10x[+++++s][-----s]<10x[++++s][----s]<10
  s=x>10[+++s][---s]x>10[++s][--s]x>10[+s][-s]
  t=<10x[+++++t][-----t]<10x[++++t][----t]<10x
  t=>10[+++t][---t]x>10[++t][--t]x>10[+t][-t]
  x=@.75f@.85f
}

Smoke {  ;
  Angle 4
  Axiom F
  F=-FF++FFFFFFFFFF
}

Spiralstring {  
  Angle 30
  Axiom d
  d=+<1d[d[|-f][|+f]-<1d[|--f][|++f]+<1d[|---f][|+++f]<1f[|----f][|++++f]]
  F=+<1d[d]-<1d[d]
}

Symmetry { ;
  Angle 4
  Axiom X
  X=+<10F[--X]-F>10[++X]+<10F[--X]-F>10[++X]
  F=<1FF>1
}
 
Symmetree { ; 
  Angle 25
  Axiom ++++++X
  X=+F<10[--X]-F>10[++X]+<10F[--X]-F>10[++X]
  F=<1FF>1
}  
