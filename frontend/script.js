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

form.addEventListener("submit", (event) => {
  event.preventDefault();

  const wordCount = getWordCount(problemInput.value);
  if (wordCount < 2) {
    problemInput.setAttribute("aria-invalid", "true");
    errorMessage.textContent = "Please enter at least 2 words.";
    problemInput.focus();
    return;
  }
  alert("Form: " + problemInput.value);

  clearError();
});