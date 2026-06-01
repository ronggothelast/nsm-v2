// Nift v2 site — minimal progressive enhancement, no framework.

(() => {
  const nav = document.querySelector('.nav');
  const toggle = document.querySelector('.nav-toggle');

  // sticky-state shadow
  if (nav) {
    const onScroll = () => nav.dataset.scrolled = window.scrollY > 12 ? 'true' : 'false';
    onScroll();
    window.addEventListener('scroll', onScroll, { passive: true });
  }

  // mobile menu
  if (toggle && nav) {
    toggle.addEventListener('click', () => {
      const open = nav.dataset.menu === 'open';
      nav.dataset.menu = open ? 'closed' : 'open';
      toggle.setAttribute('aria-expanded', String(!open));
    });
  }

  // mark current page in top nav + sidebar
  const here = location.pathname.replace(/\/$/, '') || '/index.html';
  document.querySelectorAll('.nav-links a, .docs-nav a').forEach((a) => {
    const target = (a.getAttribute('href') || '').replace(/\/$/, '');
    const norm = target.startsWith('./') ? target.slice(1) : target;
    if (
      target === here ||
      norm === here ||
      (here.endsWith('/index.html') && (target === './' || target === '/' || target === ''))
    ) {
      a.setAttribute('aria-current', 'page');
    }
  });

  // copy buttons on every <pre>
  document.querySelectorAll('pre').forEach((pre) => {
    if (pre.parentElement && pre.parentElement.classList.contains('codewrap')) return;
    const wrap = document.createElement('div');
    wrap.className = 'codewrap';
    pre.parentNode.insertBefore(wrap, pre);
    wrap.appendChild(pre);

    const btn = document.createElement('button');
    btn.className = 'copy-btn';
    btn.type = 'button';
    btn.textContent = 'Copy';
    btn.setAttribute('aria-label', 'Copy code to clipboard');
    wrap.appendChild(btn);

    btn.addEventListener('click', async () => {
      try {
        await navigator.clipboard.writeText(pre.innerText);
        btn.textContent = 'Copied';
        btn.dataset.copied = 'true';
        setTimeout(() => {
          btn.textContent = 'Copy';
          btn.dataset.copied = 'false';
        }, 1600);
      } catch {
        btn.textContent = 'Press ⌘C';
      }
    });
  });
})();
