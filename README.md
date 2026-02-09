# System Design Repository

A practical repository for learning and revising system design topics.

## How topics are organized
Each topic should live in its own folder and include:

1. `README.md` (mandatory): end-to-end design write-up.
2. `diagrams/` (recommended): architecture and sequence diagrams.
3. `implementation/` (optional): tiny proof-of-concept to demonstrate one core idea.

Use `templates/DESIGN_TEMPLATE.md` when adding a new topic.

## Current topics
- [Rate Limiter](rate-limiter/README.md)
- [Key-Value Store](key-value-store/README.md)
  - [Mini Implementation](key-value-store/implementation/README.md)

## Quality bar for each topic
- Clear functional + non-functional requirements.
- Architecture diagram + request flow.
- Data model and partitioning/replication strategy.
- Consistency model and CAP tradeoffs.
- Failure handling and scaling strategy.
- Interview-oriented tradeoffs and extensions.

## About implementations
Keep implementations small and educational:
- Focus on 1-2 core concepts (not production completeness).
- Include run instructions and key limitations.
- Add tests for core behavior if implementation exists.
