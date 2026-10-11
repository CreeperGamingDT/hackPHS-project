import { CONVERSATION_MESSAGES_API_URL } from "../api-url.js";

let messages = []; //loads from api, but for now it's empty

function colorForUser(userId) {
  let hash = 2166136261;

  for (const character of String(userId)) {
    hash ^= character.charCodeAt(0);
    hash = Math.imul(hash, 16777619);
  }

  return 220 + (hash >>> 0) % 61;
}

function formatTime(dateString) {
  return new Intl.DateTimeFormat(undefined, {
    hour: "numeric",
    minute: "2-digit",
  }).format(new Date(dateString));
}

function createMessageElement(message, previousMessage) {
  const isActiveUser = message.user_id === requestUserId;
  const isContinuation =
    previousMessage && previousMessage.user_id === message.user_id;
  const item = document.createElement("li");
  const meta = document.createElement("div");
  const author = document.createElement("span");
  const time = document.createElement("time");
  const bubble = document.createElement("p");

  item.className = isActiveUser ? "message is-active" : "message";
  if (isContinuation) {
    item.classList.add("is-continuation");
  }
  if (!isActiveUser) {
    item.style.setProperty("--user-hue", colorForUser(message.user_id));
  }

  meta.className = "message-meta";
  author.className = "message-author";
  author.textContent = isActiveUser ? "You" : `Anonymous User ${message.user_id}`;
  time.className = "message-time";
  time.dateTime = message.date;
  time.textContent = formatTime(message.date);
  bubble.className = "message-bubble";
  bubble.textContent = message.content;

  meta.append(author, time);
  if (isContinuation) {
    meta.hidden = true;
    author.hidden = true;
    time.hidden = true;
  }
  item.append(meta, bubble);
  return item;
}
const urlparams = new URLSearchParams(window.location.search);

const requestConversationId = urlparams.get("conversation_id");
const requestUserId = parseInt(urlparams.get("user_id"));

messages = await fetch(
  `${CONVERSATION_MESSAGES_API_URL}/get?conversation_id=${requestConversationId}`,
)
  .then((response) => {
    if (!response.ok) {
      throw new Error(`Request failed: ${response.status}`);
    }
    return response.json();
  })
  .catch((error) => {
    console.error("Could not load conversation messages:", error);
    return [];
  });

const messageList = document.querySelector("#message-list");
const messageForm = document.querySelector("#message-form");
const messageInput = document.querySelector("#message-input");
const resolvedButton = document.querySelector(".resolved-button");

resolvedButton.addEventListener("click", () => {
  location.href = `/frontend/?user_id=${encodeURIComponent(requestUserId)}&utm_source=chat_interface`;
});

messages
  .slice()
  .sort((first, second) => new Date(first.date) - new Date(second.date))
  .forEach((message, index, sortedMessages) =>
    messageList.append(createMessageElement(message, sortedMessages[index - 1])),
  );

messageForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  await fetch(`${CONVERSATION_MESSAGES_API_URL}/send`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      conversation_id: requestConversationId,
      user_id: requestUserId,
      message: messageInput.value,
    }),
  });
  location.reload();
});