;(function(window, undefined){
	"use strict"


	if (1 === 2) { // to run ColorPicker on its own....
		myColor = window.myColor = new window.ColorPicker({
			imagePath: 'images/' // IE8-
			// customCSS: true
		});
		return;
	}

	// Some common use variables
	var ColorPicker = window.ColorPicker,
		Tools = ColorPicker || window.Tools, // provides functions like addEvent, ... getOrigin, etc.
		colorSourceSelector = "ColorPicker", //document.getElementById('description').getElementsByTagName('select')[0],
		startPoint,
		currentTarget,
		currentTargetWidth = 0,
		currentTargetHeight = 0;


	/* ---------------------------------- */
	/* ------ Render contrast patch ----- */
	/* ---------------------------------- */


	/* ---------------------------------- */
	/* ------- Render color values ------ */
	/* ---------------------------------- */
	var colorValues = document.getElementById('colorValues'),
		renderColorValues = function(color) { // used in renderCallback of 'new ColorPicker'
			var RND = color.RND;
			console.log('render v');
			colorValues.firstChild.data =
				'rgba(' + RND.rgb.r  + ',' + RND.rgb.g  + ',' + RND.rgb.b  + ',' + color.alpha + ')' + "\n" +
				'hsva(' + RND.hsv.h  + ',' + RND.hsv.s  + ',' + RND.hsv.v  + ',' + color.alpha + ')' + "\n" +
				'hsla(' + RND.hsl.h  + ',' + RND.hsl.s  + ',' + RND.hsl.l  + ',' + color.alpha + ')' + "\n" +
				'CMYK(' + RND.cmyk.c + ',' + RND.cmyk.m + ',' + RND.cmyk.y + ',' + RND.cmyk.k + ')' + "\n" +
				'CMY('  + RND.cmy.c  + ',' + RND.cmy.m  + ',' + RND.cmy.y  + ')' + "\n" +
				'Lab('  + RND.Lab.L  + ',' + RND.Lab.a  + ',' + RND.Lab.b  + ')'; // + "\n" +

				// 'mixBG: '   + (color.rgbaMixBG.luminance).toFixed(10) + "\n" +
				// 'mixBGCBG: '   + (color.rgbaMixCustom.luminance).toFixed(10);
				// 'XYZ('  + RND.XYZ.X  + ',' + RND.XYZ.Y  + ',' + RND.XYZ.Z  + ')';
		};


	var doRender = function(color) {
			// colorModel.innerHTML = displayModel(color); // experimental
		},
		renderTimer,
		// those functions are in case there is no ColorPicker but only Colors involved
		startRender = function(oneTime){
			if (isColorPicker) { // ColorPicker present
				myColor.startRender(oneTime);
			} else if (oneTime) { // only Colors is instanciated
				doRender(myColor.colors);
			} else {
				renderTimer = window.setInterval(
					function() {
						doRender(myColor.colors);
					// http://stackoverflow.com/questions/2940054/
					}, 13); // 1000 / 60); // ~16.666 -> 60Hz or 60fps
			}
		},
		stopRender = function(){
			if (isColorPicker) {
				myColor.stopRender();
			} else {
				window.clearInterval(renderTimer);
			}
		},
		color_ColorPicker = new (ColorPicker || Colors)({
			customBG: '#808080',
			imagePath: 'images/',
			renderCallback:  function(e, value){
				var col = e.RND.rgb;
				col.alpha = e.alpha;
				var uint16 = (((31*(col.r+4))/255)<<11) |
               (((63*(col.g+2))/255)<<5) |
               ((31*(col.b+4))/255);
				var uint16hex = uint16.toString(16).toUpperCase();

				try {
					document.getElementById("uint16val").innerHTML = "0x" + uint16hex + "<br>"+uint16;
				} catch (e) {}

				//socket.emit("set-color",col);
			}
			/*actionCallback: function(e, value){
				var col= e.RND;
				console.log(col)
			},*/
		}),
		color_Colors = new Colors(),
		myColor,
		isColorPicker = colorSourceSelector.value === 'ColorPicker';

		myColor = window.myColor = color_Colors;
		// color_ColorPicker.nodes.colorPicker.style.cssText = '';
		// mySecondColor = window.mySecondColor = new ColorPicker({instanceName: 'mySecondColor'});

	// in case ColorPicker is not there...
	if (!isColorPicker) { // initial rendering
		doRender(myColor.colors);
	}



})(window);
