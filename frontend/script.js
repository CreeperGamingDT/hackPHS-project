const form = document.querySelector("#star-form");
const problemInput = document.querySelector("#problem");
const errorMessage = document.querySelector("#problem-error");

function getWordCount(value) {
  return value.trim().split(/\s+/).filter(Boolean).length;
}

function clearError() {
  problemInput.removeAttribute("aria-invalid");
  errorMessage.textContent = "";
}

problemInput.addEventListener("input", clearError);

const urlSearchParams = new URLSearchParams(window.location.search);
const requestUserId = urlSearchParams.get("user_id") || Math.round(Math.random() * 9999); // Random user_id if not provided

form.addEventListener("submit", (event) => {
  event.preventDefault();

  const wordCount = getWordCount(problemInput.value);
  if (wordCount < 2) {
    problemInput.setAttribute("aria-invalid", "true");
    errorMessage.textContent = "Please enter at least 2 words.";
    problemInput.focus();
    return;
  }
  location.href = `star-constellation/?problem=${encodeURIComponent(problemInput.value)}&user_id=${encodeURIComponent(requestUserId)}`;

  clearError();
});