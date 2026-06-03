# Template Inheritance

Nift v2 supports template inheritance — define a layout once, reuse across pages.

## Basic Usage

### Layout File (`layouts/base.html`)

```html
<!DOCTYPE html>
<html>
<head>
  <title>@yield("title")</title>
</head>
<body>
  @yield("content")
</body>
</html>
```

### Page File (`pages/about.html`)

```html
@extends("layouts/base.html")

@section("title") {
  About Us
}

@section("content") {
  <h1>About Our Company</h1>
  <p>We build amazing things.</p>
}
```

### Output

```html
<!DOCTYPE html>
<html>
<head>
  <title>About Us</title>
</head>
<body>
  <h1>About Our Company</h1>
  <p>We build amazing things.</p>
</body>
</html>
```

## Directives

### `@extends("path")`

Declares the parent layout. Must be the first directive in the child template.

```
@extends("layouts/base.html")
```

### `@section("name") { ... }`

Defines a named content block. The body is evaluated and stored under the section name.

```
@section("sidebar") {
  <nav>Sidebar content</nav>
}
```

You can also use `@endsection` instead of `}`:

```
@section("sidebar")
  <nav>Sidebar content</nav>
@endsection
```

### `@yield("name")`

Renders a section's content in the layout. Used in parent templates only.

```
@yield("sidebar")
```

### `@parent`

References the parent's section content. Used in child sections that want to extend (not replace) the parent's section.

```
@section("footer") {
  @parent
  <p>Additional footer content</p>
}
```

## How It Works

1. **Parse child**: `@extends` stores layout path, `@section` blocks store content as variables
2. **Parse layout**: Layout is a regular template
3. **Evaluate**: Section content is stored as `__section_<name>` variables
4. **Render layout**: `@yield("name")` reads and renders the section variable

## Limitations

- **Single-level inheritance only** — no nested extends (layout extends base layout)
- **Sections must have unique names** within a template
- **`@parent` is a no-op** — parent section content is not automatically injected

## Example: Full Page Template

```html
@extends("layouts/base.html")

@section("title") {
  @title
}

@section("head") {
  <link rel="stylesheet" href="/css/custom.css">
}

@section("content") {
  <article>
    <h1>@title</h1>
    @content
  </article>
}

@section("scripts") {
  <script src="/js/app.js"></script>
}
```

## Example: Multiple Pages from One Layout

**Layout** (`layouts/blog.html`):
```html
<html>
<body>
  @yield("header")
  <main>@yield("content")</main>
  <footer>@yield("footer")</footer>
</body>
</html>
```

**Page 1** (`pages/post1.html`):
```html
@extends("layouts/blog.html")
@section("header") { <h1>Post 1</h1> }
@section("content") { <p>First post</p> }
@section("footer") { <p>© 2026</p> }
```

**Page 2** (`pages/post2.html`):
```html
@extends("layouts/blog.html")
@section("header") { <h1>Post 2</h1> }
@section("content") { <p>Second post</p> }
@section("footer") { <p>© 2026</p> }
```
