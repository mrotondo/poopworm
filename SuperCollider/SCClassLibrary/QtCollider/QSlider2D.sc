QSlider2D : QAbstractStepValue {
  *qtClass { ^"QcSlider2D" }

  *new { arg parent, bounds;
    var me = super.new( parent, bounds );
    me.connectMethod( "randomize()", \randomize );
    ^me;
  }

  x {
    ^this.getProperty( \xValue );
  }

  x_ { arg aFloat;
    this.setProperty( \xValue, aFloat );
  }

  activex_ { arg aFloat;
    this.x_(aFloat);
    this.doAction;
  }

  y {
    ^this.getProperty( \yValue );
  }

  y_ { arg aFloat;
    this.setProperty( \yValue, aFloat );
  }

  activey_ { arg aFloat;
    this.y_(aFloat);
    this.doAction;
  }

  setXY { arg x, y;
    this.x_(x);
    this.y_(y);
  }

  setXYActive { arg x, y;
    this.setXY(x,y);
    this.doAction;
  }

  randomize {
    this.setXYActive( 1.0.rand, 1.0.rand );
  }

  knobColor {
    ^this.palette.baseTextColor;
  }

  knobColor_ { arg color;
    this.setProperty( \palette, this.palette.baseTextColor_(color) );
  }

  background {
    ^this.palette.baseColor;
  }

  background_ { arg color;
    this.setProperty( \palette, this.palette.baseColor_(color) );
  }
}
