window.onload = function(){ 
	var button = document.getElementById("clickme");
	var audio = document.getElementById("myAudio"); 
	var count = 0;
    button.onclick = function() {
		count += 1;
		button.innerHTML = /* "Click me: " + */ count;
		audio.play();
	};
};

