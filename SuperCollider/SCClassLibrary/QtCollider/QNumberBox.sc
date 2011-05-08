QNumberBox : QAbstractStepValue {
  var <clipLo, <clipHi, <scroll, <scroll_step, <decimals;
  var <align, <buttonsVisible = false;
  var <normalColor, <typingColor;

  *qtClass { ^"QcNumberBox" }

  *new { arg aParent, aBounds;
    var obj = super.new( aParent, aBounds );
    obj.initQNumberBox;
    ^obj;
  }

  initQNumberBox {
    clipLo = inf;
    clipHi = inf;
    scroll = true;
    scroll_step = 1;
    normalColor = Color.black;
    typingColor = Color.red;
  }

  clipLo_ { arg aFloat;
    clipLo = aFloat;
    this.setProperty( \minimum, aFloat; );
  }

  clipHi_ { arg aFloat;
    clipHi = aFloat;
    this.setProperty( \maximum, aFloat; );
  }

  scroll_ { arg aBool;
    scroll = aBool;
    this.setProperty( \scroll, aBool );
  }

  scroll_step_ { arg aFloat;
    scroll_step = aFloat;
    this.setProperty( \scrollStep, aFloat );
  }

  decimals_ {  arg anInt;
    decimals = anInt;
    this.setProperty( \decimals, anInt );
  }

  align_ { arg alignment;
    align = alignment;
    this.setProperty( \alignment, QAlignment(alignment));
  }

  stringColor {
    ^this.palette.baseTextColor;
  }

  stringColor_ { arg color;
    this.setProperty( \palette, this.palette.baseTextColor_(color) );
  }

  normalColor_ { arg aColor;
    normalColor = aColor;
    this.setProperty( \normalColor, aColor );
  }

  typingColor_ { arg aColor;
    typingColor = aColor;
    this.setProperty( \editingColor, aColor );
  }

  background {
    ^this.palette.baseColor;
  }

  background_ { arg color;
    this.setProperty( \palette, this.palette.baseColor_(color) )
  }

  buttonsVisible_ { arg aBool;
    buttonsVisible = aBool;
    this.setProperty( \buttonsVisible, aBool );
  }
}
