# postgres

A wrapper around Postgres SQL database that can be used by Cxy applications

## Usage

Import this library in your Cxy project:

```cxy
import { hello } from "@postgres"

func main() {
  println(hello("World"))
}
```

## Testing

```bash
cxy package test
```
