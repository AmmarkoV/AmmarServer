(function () {
  'use strict';

  var pollIDField = document.getElementById('pollID');
  if (!pollIDField) { return; }
  var pollID = pollIDField.value;

  var resultsGrid = document.getElementById('resultsGrid');
  var nameField = document.getElementById('nameField');

  function refreshResults() {
    // Don't yank the grid out from under someone who is actively typing their name / picking votes ,
    // it only matters for *other* people's rows anyway , which is what this refresh is for.
    if (document.activeElement === nameField) { return; }

    var x = new XMLHttpRequest();
    x.open('GET', 'pollResults.html?poll=' + encodeURIComponent(pollID) + '&_=' + Date.now(), true);
    x.onreadystatechange = function () {
      if (x.readyState === 4 && x.status === 200 && resultsGrid) {
        resultsGrid.innerHTML = x.responseText;
      }
    };
    x.send(null);
  }

  setInterval(refreshResults, 4000);
})();
