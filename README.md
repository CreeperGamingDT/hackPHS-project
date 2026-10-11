# StarMind

HackPHS 2026 project. The project is made up of:

- **Static frontend** in [`./frontend/`](./frontend/), which contains the
  StarMind problem input, constellation view, and chat interface.
- **Similar-problems service** in
  [`./backend/problems-storing/`](./backend/problems-storing/), a C++ HTTP
  server that stores submitted problems in `problems.txt` ("our CSV database").
- **Chat service** in [`./backend/chat-interface/`](./backend/chat-interface/), a
  FastAPI service backed by PostgreSQL.

## Demo link
[Link](https://curly-space-waddle-vxvw9vxwqvjcwjxr-5502.app.github.dev/frontend/)

This link will not be available 24/7, but it will be when we run our codespace server

## Prerequisites

Install the following before starting the project:

- Python 3.10 or newer
- Docker
- A C++ compiler with POSIX socket support, such as `g++`
- A static file server (Python's built-in server is sufficient)

## Run the backend services

Start each backend in a separate terminal.

### 1. Similar-problems service

The C++ service listens on `localhost:5000` and reads/writes to
`./backend/problems-storing/problems.txt`.

```bash
cd ./backend/problems-storing
g++ -std=c++17 -Wall -Wextra -o Echo Echo.cpp
./Echo
```

Keep this terminal running. The service accepts:

```text
POST http://localhost:5000/similar_problems
```

with a JSON body such as:

```json
{"problem":"I am stressed about my math test","user_id":1234}
```

### 2. Chat service

Start PostgreSQL in Docker:

```bash
docker run --name postgres \
  -e POSTGRES_DB=postgres \
  -e POSTGRES_USER=postgres \
  -e POSTGRES_PASSWORD=postgres \
  -p 5432:5432 \
  -d postgres:16
```

Wait until PostgreSQL is ready, then initialize its schema and sample data
through the container:

```bash
until docker exec postgres pg_isready -U postgres -d postgres; do
  sleep 1
done
docker exec -i postgres psql -U postgres -d postgres \
  < ./backend/chat-interface/schema.sql
```

Create and activate the container, install the Python dependencies,
and start FastAPI:

```bash
cd ./backend/chat-interface
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
export DATABASE_URL="postgresql://postgres:postgres@localhost:5432/postgres"
uvicorn app:app --reload --host 0.0.0.0 --port 8000
```

The chat API is available at `http://localhost:8000`.

### Reset the database to the schema sample data

To permanently delete all tables and data, then recreate only the tables and
sample rows from
[`backend/chat-interface/schema.sql`](./backend/chat-interface/schema.sql),
stop FastAPI first and run:

```bash
docker exec postgres psql -U postgres -d postgres \
  -c 'DROP SCHEMA public CASCADE; CREATE SCHEMA public;'
docker exec -i postgres psql -U postgres -d postgres \
  < ./backend/chat-interface/schema.sql
```

This resets the existing database without removing the Docker container. The
schema script then recreates the sample conversations and messages.


## Configure the frontend for local services

The backend API URLs are defined in
[`./frontend/api-url.js`](./frontend/api-url.js). The values point to
the project's backend service endpoints for our demo codespace. For a local run, change them to:

```js
export const SIMILAR_PROBLEMS_API_URL =
  "http://localhost:5000/similar_problems";
export const CONVERSATION_MESSAGES_API_URL =
  "http://localhost:8000";
```

## Run the frontend

From the repository root, start a static server so that ES modules and browser
requests work correctly:

```bash
python3 -m http.server 5502
```

Open the main page at
[`http://localhost:5502/frontend/`](http://localhost:5502/frontend/).

## Our personal comments
The project is pretty rushed (only had 24 hours) with many hardcoded values and many security vulnerabilities such as viewing another users conversation, manually changing your user_id, and being able to send unlimited problems of any text. But the baseline idea and functionality still remains, and is useful if no one hacks our app.