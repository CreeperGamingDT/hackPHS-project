const ACTIVE_USER_ID = 12345;

const messages = [
  {
    date: "2026-10-10T09:12:00Z",
    user_id: 48271,
    content: "Anyone else watching the meteor shower tonight?",
  },
  {
    date: "2026-10-10T09:13:00Z",
    user_id: ACTIVE_USER_ID,
    content: "Absolutely. The sky should be clear out here.",
  },
  {
    date: "2026-10-10T09:15:00Z",
    user_id: 93602,
    content: "I found a quiet spot just beyond the ridge.",
  },
  {
    date: "2026-10-10T09:16:00Z",
    user_id: 7184,
    content: "Save me a place. Bringing the star charts!",
  },
  {
    date: "2026-10-10T09:18:00Z",
    user_id: ACTIVE_USER_ID,
    content: "See you there in a few minutes.",
  },
];

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

function createMessageElement(message) {
  const isActiveUser = message.user_id === ACTIVE_USER_ID;
  const item = document.createElement("li");
  const meta = document.createElement("div");
  const author = document.createElement("span");
  const time = document.createElement("time");
  const bubble = document.createElement("p");

  item.className = isActiveUser ? "message is-active" : "message";
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
  item.append(meta, bubble);
  return item;
}

const messageList = document.querySelector("#message-list");
const messageForm = document.querySelector("#message-form");
const messageInput = document.querySelector("#message-input");

messages
  .slice()
  .sort((first, second) => new Date(first.date) - new Date(second.date))
  .forEach((message) => messageList.append(createMessageElement(message)));

messageForm.addEventListener("submit", (event) => {
  event.preventDefault();
  window.alert("You sent: " + messageInput.value);
});