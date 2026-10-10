import os
from datetime import timezone

import psycopg
from fastapi import FastAPI, HTTPException, Query
from pydantic import BaseModel, Field
from psycopg.rows import dict_row

app = FastAPI(title="Chat API")


class SendMessage(BaseModel):
    conversation_id: int = Field(gt=0)
    user_id: int = Field(gt=0)
    message: str = Field(min_length=1)


def get_connection():
    database_url = os.environ.get("DATABASE_URL")
    if not database_url:
        raise RuntimeError("DATABASE_URL environment variable is required")
    return psycopg.connect(database_url, row_factory=dict_row)


def serialize_message(row):
    return {
        "date": row["created_at"].astimezone(timezone.utc).strftime(
            "%Y-%m-%dT%H:%M:%SZ"
        ),
        "user_id": row["user_id"],
        "content": row["content"],
    }


@app.get("/get")
def get_messages(conversation_id: int = Query(gt=0)):
    with get_connection() as connection:
        with connection.cursor() as cursor:
            cursor.execute(
                "SELECT 1 FROM conversations WHERE id = %s",
                (conversation_id,),
            )
            if cursor.fetchone() is None:
                raise HTTPException(status_code=404, detail="Conversation not found")

            cursor.execute(
                """
                SELECT created_at, user_id, content
                FROM messages
                WHERE conversation_id = %s
                ORDER BY created_at, id
                """,
                (conversation_id,),
            )
            return [serialize_message(row) for row in cursor.fetchall()]


@app.post("/send", status_code=201)
def send_message(payload: SendMessage):
    with get_connection() as connection:
        with connection.cursor() as cursor:
            cursor.execute(
                """
                INSERT INTO messages (conversation_id, user_id, content)
                SELECT id, %s, %s
                FROM conversations
                WHERE id = %s
                RETURNING created_at, user_id, content
                """,
                (payload.user_id, payload.message, payload.conversation_id),
            )
            row = cursor.fetchone()
            if row is None:
                raise HTTPException(status_code=404, detail="Conversation not found")
            return serialize_message(row)