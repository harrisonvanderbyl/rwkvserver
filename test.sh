
curl http://localhost:8080/v1/completions \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $OPENAI_API_KEY" \
  -d '{
    "model": "1B5",
    "prompt": "User: Say this is a test. Respond in JSON.\n\nAssisstant:",
    "max_tokens": 20,
    "temperature": 1,
    "stop": []
   }'\
   --no-buffer

