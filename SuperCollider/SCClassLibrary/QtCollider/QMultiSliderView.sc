QMultiSliderView : QView {
  var <size=0, <editable=true, <step=0;
  var <reference;
  var <indexIsHorizontal=true, <elasticMode=false;
  var <indexThumbSize=12, <valueThumbSize=12, <gap=1;
  var <drawLines=false, <drawRects=true;
  var <metaAction;


  *qtClass { ^"QcMultiSlider" }

  background {
    ^this.palette.baseColor;
  }

  background_ { arg color;
    this.setProperty( \palette, this.palette.baseColor_(color) );
  }

  size_ { arg int;
    size = int;
    this.setProperty( \size, int );
  }

  indexIsHorizontal_ { arg bool;
    indexIsHorizontal = bool;
    if( bool ) {
      this.setProperty( \orientation, QOrientation(\horizontal) );
    } {
      this.setProperty( \orientation, QOrientation(\vertical) );
    };
  }

  editable_ { arg aBool;
    editable = aBool;
    this.setProperty( \editable, aBool );
  }

  readOnly {
    ^editable.not;
  }

  readOnly_ { arg bool;
    this.editable_( bool.not );
  }

  step_ { arg aFloat;
    step = aFloat;
    this.setProperty( \stepSize, aFloat );
  }

  value {
    ^this.getProperty( \values );
  }

  value_ { arg floatArray;
    this.setProperty( \values, floatArray );
  }

  valueAction_ { arg val;
    this.value_(val);
    action.value(this);
  }

  currentvalue {
    ^this.getProperty( \value );
  }

  currentvalue_ { arg aFloat;
    this.setProperty( \value, aFloat );
  }

  index {
    ^this.getProperty( \index );
  }

  index_ { arg anInt;
    this.setProperty( \index, anInt );
  }

  selectionSize {
    ^this.getProperty( \selectionSize );
  }

  selectionSize_ { arg anInt;
    this.setProperty( \selectionSize, anInt );
  }

  reference_ { arg aFloatArray;
    reference = aFloatArray;
    this.setProperty( \reference, aFloatArray );
  }

  startIndex_ { arg anInt;
    this.setProperty( \startIndex, anInt );
  }

  elasticMode_ { arg int;
    elasticMode = int.booleanValue;
    this.setProperty( \elastic, elasticMode);
  }

  thumbSize_ { arg float;
    this.indexThumbSize_(float);
    this.valueThumbSize_(float);
  }

  indexThumbSize_ { arg float;
    indexThumbSize = float;
    this.setProperty( \indexThumbSize, float );
  }

  valueThumbSize_ { arg float;
    valueThumbSize = float;
    this.setProperty( \valueThumbSize, float );
  }

  gap_ { arg anInt;
    gap = anInt;
    this.setProperty( \gap, anInt );
  }

  // alias for 'gap'
  xOffset_ { arg int; this.gap_(int); }
  xOffset { arg int; ^this.gap; }

  drawLines_ { arg bool;
    drawLines = bool;
    this.setProperty( \drawLines, bool );
  }

  drawRects_ { arg bool;
    drawRects = bool;
    this.setProperty( \drawRects, bool );
  }

  showIndex_ { arg aBool;
    this.setProperty( \highlight, aBool );
  }

  isFilled_ { arg aBool;
    this.setProperty( \isFilled, aBool );
  }

  fillColor_ { arg aColor;
    this.setProperty( \fillColor, aColor );
  }

  strokeColor_ { arg aColor;
    this.setProperty( \strokeColor, aColor );
  }

  colors_ { arg colorStroke, colorFill;
    this.strokeColor_( colorStroke );
    this.fillColor_ ( colorFill );
  }

  metaAction_ { arg func;
    this.manageMethodConnection( metaAction, func, 'metaAction()', \doMetaAction );
    metaAction = func;
  }

  doMetaAction {
    metaAction.value(this);
  }
}
