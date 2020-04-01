x1 = null;
y1 = null;

x2 = null;
y2 = null;

function initJcrop() {
    $('#image_wrapper').Jcrop({
        onChange: showCoords,
        //onSelect:   showCoords
        onSelect: setCoords
    });

    divWidth = document.getElementById("image_wrapper").offsetWidth;
    imgWidth = document.getElementById("imageToCrop").naturalWidth;
    ratio = imgWidth / divWidth;

    console.log(document.getElementById("image_wrapper").offsetWidth);
    console.log(document.getElementById("imageToCrop").naturalWidth);
    console.log(ratio);
};

// Simple event handler, called from onChange and onSelect
// event handlers, as per the Jcrop invocation above
function showCoords(c){
    console.log(c)
    console.log('x='+ c.x * ratio +' y='+ c.y * ratio +' x2='+ c.x2 * ratio +' y2='+ c.y2 * ratio )
    console.log('w='+c.w * ratio  +' h='+ c.h * ratio )
};

function setCoords(c){
    x1 = c.x * ratio ;
    y1 = c.y * ratio ;

    x2 = c.x2 * ratio ;
    y2 = c.y2 * ratio ;

    console.log('Set coordinates to: x1='+ x1 +' y1='+ y1 +' x2='+ x2 +' y2='+ y2)

    /*
    // Log the response
    console.log("Writing answer for session " + session);
    //console.log("ANS: " +  resp + ", time: " + response_time + ", flag: " + flagVal + ", flagDATA: " + flagData);
    $.ajax({
    url: "php/logRegion.php",
    type: "POST",
    data: {  session: session, x1: c.x1, y1: c.y1, x2: c.x2, y2: c.y2, image: imageName, label: "test" },
    dataType: "text",
    success: function(d) {
      alert("Answer submitted. Thanks!");

      // Submit the HIT
      //submitToTurk(undefined, {"useAlert": false});
    }
    });
    */

    addResponseHooks();
}