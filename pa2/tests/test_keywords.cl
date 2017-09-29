-- test keywords
class else  fi if in inherits isvoid let loop pool then while case esac new of not
CLASS ELSE  FI IF IN INHERITS ISVOID LET LOOP POOL THEN WHILE CASE ESAC NEW OF NOT
Class Else  Fi If In Inherits Isvoid Let Loop Pool Then While Case Esac New Of Not
-- test booleans (treated differently)
true false truE falsE
-- considered to be TYPE_ID's:
True False

-- test operators, etc
+ - * / < ( ) { } <= <- => = not ~ @ . : ;
+-*/<(){}<=<-=>=not~@.:;
-- not allowed:
_ #
_identifier_"string_const"

-- potentially ambiguous combos
<=><-><==><<==