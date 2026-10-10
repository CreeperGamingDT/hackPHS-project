# Chat Backend Interface

Python chat API backed by PostgreSQL. It provides message retrieval and sending
for group conversations; it does not implement authentication or conversation
access restrictions.

## Setup

Use Python 3.10+ and PostgreSQL. From `chat-interface/`, install dependencies,
create a PostgreSQL database, and apply the schema and sample data:

```sh
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
export DATABASE_URL='postgresql://postgres:password@localhost:5432/chat'
psql "$DATABASE_URL" -f schema.sql
uvicorn app:app --reload
```

The sample data creates group conversations `67890` (Weekend Plans) and
`67891` (Project Team). The API documentation is available at `/docs` when the
server is running.

## API

`GET /get?conversation_id=67890` returns the conversation's messages, ordered
oldest first:

```json
[
	{
		"date": "2026-10-10T09:00:00Z",
		"user_id": 12345,
		"content": "Hello everybody!"
	}
]
```

`POST /send` accepts JSON and returns the saved message with HTTP 201:

```json
{
	"conversation_id": 67890,
	"user_id": 12345,
	"message": "Hello everybody!"
}
```

`conversation_id` is required to select which group receives the message. The
response uses the same `date`, `user_id`, and `content` fields as `GET /get`.
Unknown conversation IDs return HTTP 404; invalid request fields return HTTP
422.
