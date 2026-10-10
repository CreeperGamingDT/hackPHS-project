const POP_DURATION = 300;
const LINE_DURATION = 1000;
const FINAL_DURATION = 1000;
const FALLBACK_STAR_COUNT = 8;
const SCENE_DROP = 56;
const STAR_SIZE = 7;
const LINE_COLOR = "#fff"
const LINE_SIZE = 2;

const canvas = document.querySelector("#constellation-canvas");
const joinButton = document.querySelector("#join-button");
const tooltip = document.querySelector("#star-tooltip");
const problemText = document.querySelector("#star-problem");
const ownerText = document.querySelector("#star-owner");
let stars = [];
let animationStart = 0;
let canvasWidth = 0;
let canvasHeight = 0;
let sceneOffset = 0;
let activeStar = null;

const SAMPLE_PROBLEMS = [
  "I failed my math test and I don't know how to improve.",
  "I feel anxious about speaking in front of the class.",
  "I don't understand the science experiment we did in class.",
  "I need help with my reading comprehension.",
  "I feel left out when my friends play without me.",
  "I don't know how to solve this word problem.",
  "I feel nervous about taking the upcoming quiz.",
  "I need help organizing my homework assignments.",
]


if (!canvas || !joinButton || !tooltip || !problemText || !ownerText) {
  throw new Error("The constellation scene could not be initialized.");
}

const context = canvas.getContext("2d");
if (!context) {
  throw new Error("A 2D canvas context could not be created.");
}

function getStarCount() {
  const requestedCount = Number.parseInt(
    window.prompt("How many stars should join the constellation?", FALLBACK_STAR_COUNT),
    10,
  );

  return Number.isInteger(requestedCount) && requestedCount > 0
    ? requestedCount
    : FALLBACK_STAR_COUNT;
}

function createStars(count) {
  const positions = createStarPositions(count);
  return positions.map((position) => ({
    ...position,
    problem: SAMPLE_PROBLEMS[Math.floor(Math.random() * SAMPLE_PROBLEMS.length)],
    user_id: Math.floor(Math.random() * 10000),
  }));
}

function createStarPositions(count) {
  const random = createSeededRandom(count * 7919);
  const horizontalPadding = Math.min(canvasWidth * 0.12, 160);
  const top = Math.max(canvasHeight * 0.25, 150);
  const bottom = Math.max(top + 1, canvasHeight * 0.78);

  return Array.from({ length: count }, () => ({
    x: horizontalPadding + random() * Math.max(canvasWidth - horizontalPadding * 2, 1),
    y: top + random() * Math.max(bottom - top, 1),
  }));
}

function repositionStars() {
  const positions = createStarPositions(stars.length);
  stars.forEach((star, index) => {
    star.x = positions[index].x;
    star.y = positions[index].y;
  });
}

function colorForUser(userId) {
  let hash = 2166136261;
  for (const character of String(userId)) {
    hash ^= character.charCodeAt(0);
    hash = Math.imul(hash, 16777619);
  }
  return 220 + (hash >>> 0) % 61;
}

function createSeededRandom(seed) {
  let value = seed >>> 0;
  return () => {
    value += 0x6d2b79f5;
    let result = value;
    result = Math.imul(result ^ (result >>> 15), result | 1);
    result ^= result + Math.imul(result ^ (result >>> 7), result | 61);
    return ((result ^ (result >>> 14)) >>> 0) / 4294967296;
  };
}

function easeOutBack(progress) {
  const overshoot = 1.70158;
  const adjustedOvershoot = overshoot + 1;
  return 1 + adjustedOvershoot * (progress - 1) ** 3 + overshoot * (progress - 1) ** 2;
}

function easeInOut(progress) {
  return progress < 0.5
    ? 2 * progress ** 2
    : 1 - ((-2 * progress + 2) ** 2) / 2;
}

function resizeCanvas() {
  const pixelRatio = Math.min(window.devicePixelRatio || 1, 2);
  canvasWidth = window.innerWidth;
  canvasHeight = window.innerHeight;
  canvas.width = Math.floor(canvasWidth * pixelRatio);
  canvas.height = Math.floor(canvasHeight * pixelRatio);
  canvas.style.width = `${canvasWidth}px`;
  canvas.style.height = `${canvasHeight}px`;
  context.setTransform(pixelRatio, 0, 0, pixelRatio, 0, 0);
  if (stars.length > 0) {
    repositionStars();
  }
}

function getStarAtPoint(x, y) {
  return stars.find((star) => {
    const distanceX = x - star.x;
    const distanceY = y - (star.y + sceneOffset);
    return Math.hypot(distanceX, distanceY) <= STAR_SIZE * 2.5;
  });
}

function updateTooltipPosition() {
  if (!activeStar) {
    return;
  }

  tooltip.style.left = `${activeStar.x}px`;
  tooltip.style.top = `${activeStar.y + sceneOffset - STAR_SIZE}px`;
}

function showTooltip(star) {
  if (!star) {
    hideTooltip();
    return;
  }

  if (activeStar === star && tooltip.classList.contains("is-visible")) {
    updateTooltipPosition();
    return;
  }

  activeStar = star;
  problemText.textContent = `"${star.problem}"`;
  ownerText.textContent = `Anonymous User ${star.user_id}`;
  ownerText.style.color = `hsl(${colorForUser(star.user_id)} 78% 72%)`;
  updateTooltipPosition();
  tooltip.classList.remove("is-visible");
  window.requestAnimationFrame(() => tooltip.classList.add("is-visible"));
}

function hideTooltip() {
  activeStar = null;
  tooltip.classList.remove("is-visible");
}

function pointerPosition(event) {
  const bounds = canvas.getBoundingClientRect();
  return {
    x: event.clientX - bounds.left,
    y: event.clientY - bounds.top,
  };
}

function drawBackground() {
  const background = context.createRadialGradient(
    canvasWidth * 0.5,
    canvasHeight * 0.45,
    0,
    canvasWidth * 0.5,
    canvasHeight * 0.45,
    Math.max(canvasWidth, canvasHeight) * 0.8,
  );
  background.addColorStop(0, "#051228");
  background.addColorStop(0.5, "#07132e");
  background.addColorStop(1, "#02050f");
  context.fillStyle = background;
  context.fillRect(0, 0, canvasWidth, canvasHeight);
}

function drawStar(star, scale, opacity, offset) {
  if (opacity <= 0) {
    return;
  }

  context.save();
  context.globalAlpha = opacity;
  context.fillStyle = "#ffffff";
  context.beginPath();
  context.arc(star.x, star.y + offset, Math.max(scale * STAR_SIZE, 0), 0, Math.PI * 2);
  context.fill();
  context.restore();
}

function drawLine(from, to, progress, offset) {
  if (progress <= 0) {
    return;
  }

  const endX = from.x + (to.x - from.x) * progress;
  const endY = from.y + (to.y - from.y) * progress + offset;
  context.strokeStyle = LINE_COLOR;
  context.lineWidth = LINE_SIZE;
  context.beginPath();
  context.moveTo(from.x, from.y + offset);
  context.lineTo(endX, endY);
  context.stroke();
}

function render(timestamp) {
  if (!animationStart) {
    animationStart = timestamp;
  }

  const elapsed = timestamp - animationStart;
  const revealEnd = stars.length * POP_DURATION;
  const linesEnd = revealEnd + LINE_DURATION;
  const finalProgress = Math.min(Math.max((elapsed - linesEnd) / FINAL_DURATION, 0), 1);
  sceneOffset = SCENE_DROP * easeInOut(finalProgress);

  drawBackground();

  stars.forEach((star, index) => {
    const starProgress = Math.min(Math.max((elapsed - index * POP_DURATION) / POP_DURATION, 0), 1);
    drawStar(star, easeOutBack(starProgress), starProgress, sceneOffset);
  });

  if (elapsed >= revealEnd) {
    const lineProgress = Math.min((elapsed - revealEnd) / LINE_DURATION, 1);
    stars.forEach((star, index) => {
      const nextStar = stars[(index + 1) % stars.length];
      drawLine(star, nextStar, lineProgress, sceneOffset);
    });
  }

  if (elapsed >= linesEnd && !joinButton.classList.contains("is-visible")) {
    joinButton.classList.add("is-visible");
  }

  updateTooltipPosition();
  window.requestAnimationFrame(render);
}

function startAnimation() {
  resizeCanvas();
  animationStart = 0;
  window.requestAnimationFrame(render);
}

joinButton.addEventListener("click", () => {
  window.alert("User has requested to create a group");
});

canvas.addEventListener("pointermove", (event) => {
  const position = pointerPosition(event);
  showTooltip(getStarAtPoint(position.x, position.y));
});

canvas.addEventListener("pointerdown", (event) => {
  const position = pointerPosition(event);
  showTooltip(getStarAtPoint(position.x, position.y));
});

canvas.addEventListener("pointerleave", (event) => {
  if (event.pointerType === "mouse") {
    hideTooltip();
  }
});

window.addEventListener("resize", resizeCanvas);
stars = createStars(getStarCount());
startAnimation();
