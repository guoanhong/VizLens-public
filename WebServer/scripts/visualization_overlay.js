var segSizeW = null;
var segSizeH = null;
var imgName = null;
var imgPath = null;

var img = new Image();
var imgW = null;
var imgH = null;

function addVisualizationOverlays() {
	console.log(gup('workerId'));
	// Get the segment information
	$.ajax({
		url: "php/getVisualizationMasks.php",
		type: "POST",
		data: { workerid: gup('workerId'), session: session, stateid: gup('stateid') },
		dataType: "text",
		success: function(d) {
			//alert(d);
			if( d.length > 0 ) {
				dAry = d.split(";");

				for( i = 0; i < dAry.length; i++ ) {
					var elemAry = dAry[i].split(",");
					var xPos1 = parseInt(elemAry[0]);
					var yPos1 = parseInt(elemAry[1]);
					var xPos2 = parseInt(elemAry[2]);
					var yPos2 = parseInt(elemAry[3]);
					var label = elemAry[4];

					segSizeW = xPos2 - xPos1;
					segSizeH = yPos2 - yPos1;

					//alert("Result: " + segSizeW + " | " + segSizeH + " | " + xPos1 + " | " + yPos1);
					$('#image_wrapper').append("<div class='mask' style='width: " + segSizeW + "px; height: " + segSizeH + "px; left: " + xPos1 + "px; top: " + yPos1 + "px'><span class=\"maskText\">" + label + "</span></div>");
				}
			}
		}
	});
}