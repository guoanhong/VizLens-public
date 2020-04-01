// VARS
var img = new Image();
var imgW;
var imgH;

var imageName = null;
var imagePath = null;
var question = null;
var ansType = null;

var imgResponse = null;
var qResponse = null;

var flagVal = false;

//var session = gup("session") ? gup("session") : null;
session = null;
stateid = null;

// Dynamically set the question being asked
function addResponseHooks() {
	// Reveal the submit button
	$('#legion-submit').show();
	$('#legion-submit').removeAttr("disabled");
	$('#legion-submit').off('click');

	// Handle submission
	$('#legion-submit').click( function() {
		var flagData = null;

		// TODO: Check if a bound is set
		var isAnswered = false;
		if( x1 != null ) {  // WLOG, pick one coord to check
			isAnswered = true;
		}

		var complete = 1; //document.getElementById("complete").checked ? 1 : 0;
		var incomplete = 0; //document.getElementById("incomplete").checked ? 2 : 0;

		var clear = 1; //document.getElementById("clear").checked ? 1 : 0;
		var unclear = 0; //document.getElementById("unclear").checked ? 2 : 0;

		var passCheck = true;
		var feedback = "";
		// if ($('#description').val() == ""){
		// 	passCheck = false;
		// 	feedback = "You didn't put in any decription for the interface. Please try again. Thanks!";
		// }

		console.log(complete, incomplete, clear, unclear, $('#buttons').val(), $('#description').val());

		// If there is an answer...
		if( isAnswered && passCheck ) {  // TODO: Is this needed given the returns above?
			// Log the response
			console.log("Writing answer for session " + session + " state " + stateid);

			$.ajax({
				url: "php/logCrop.php",
				type: "POST",
				data: {  workerid:gup('workerId'), session: session, stateid: stateid, complete: complete + incomplete, clear: clear + unclear, x1: x1, y1: y1, x2: x2, y2: y2, image: imageName, buttons: $('#buttons').val(), description: "test" },
				dataType: "text",
				success: function(d) {
					// Submit the HIT
					alert("Answer submitted. Thanks!");
					submitToTurk(undefined, {"useAlert": false});
				}
			});
		} else {
			alert(feedback);
		}
	});
};

$(document).ready( function() {
	$('#legion-submit').hide();
	$('#interface-name').html(gup('session'));

	// Let everyone know the image is coming
	console.log("Loading image...");
	console.log("session: " + gup('session'));
	console.log("stateid: " + gup('stateid'));

    if( gup("assignmentId") != "ASSIGNMENT_ID_NOT_AVAILABLE" ) {
		// Get the image + question information
		$.ajax({
            url: "php/getOriginalImage.php",
            type: "POST",
            data: { workerid: gup('workerId'), session: gup('session'), stateid: gup('stateid'), task: 'crop'},
            dataType: "json",
            success: function(jsonResp) {  // 2-element array -- [image json, question json]
				console.log("JSON::", jsonResp);

				// TODO: load image
				imageName= jsonResp['image'];
				imagePath = "images_original/" + gup('session') + "/" + imageName;

				session = jsonResp['session'];
				stateid = jsonResp['stateid'];

				// Set an on-error reload to address 404 errors in images
				img.onerror = function() {
					//location.reload();
				};
				// Display the image
				console.log("Setting image to: ", imageName);
				img.src = imagePath;
				//location.load();
			}
		});

		// Figure out the image size
       	img.onload = function() {
			//alert("Loading...");
           	imgW = this.width;
           	imgH = this.height;

			// Load the image as a background and 
			//$('#image_wrapper').css("width", imgW);
			//$('#image_wrapper').css("height", imgH);

			// Set the image as a background
			$('#image_wrapper').html('<img id="imageToCrop" width="100%" src="' + this.src + '"></img>');
			//$('#image_wrapper').css("background-image", "url(" + this.src + ")");

			// Once the image is loaded, then we can let people respond
			//addResponseHooks();

			console.log(document.getElementById("image_wrapper").offsetWidth);
			console.log(document.getElementById("imageToCrop").naturalWidth);

			// Add region-selection functions
			initJcrop();
			//addOverlays();
		}
    }
    else {
 		$('#image_wrapper').html("<div style='text-align: center;'>Please accept the HIT to continue.</div>");
    }
});

