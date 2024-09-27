; SPDX-License-Identifier: GPL-3.0-only
;
{--- SYLVIE GALLET -------------------------------------------------------}

ismand_demo { ; by Sylvie Gallet
              ; uses the Pokorny formula z -> 1/(z^2+c)
   ;
   ; Use the spacebar to toggle between mandel and julia sets
   ;
   if (ismand)
      z = 0, c = pixel
   else
      z = pixel, c = p1
   endif
   :
   z = 1 / (z*z + c)
   |z| <= p2
}
