
comment { Formula file released with Fractint 20.0 }

{--- SYLVIE GALLET -------------------------------------------------------}

ismand_demo { ; sylvie_gallet@compuserve.com
              ; uses the Pokorny formula z -> 1/(z^2+c)
;
; Use the spacebar to toggle between mandel and julia sets
;
if (ismand)
  z = 0 , c = pixel
else
  z = pixel , c = p1
endif
:
z = 1 / (z*z + c)
|z| <= p2
}
