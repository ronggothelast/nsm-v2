/* Nift v2 docs — interactions */
(function () {
  'use strict';

  /* ── Scroll nav state ──────────────────────────────────────────── */
  const nav = document.querySelector('.nav');
  if (nav) {
    const io = new IntersectionObserver(
      ([e]) => { nav.dataset.scrolled = (!e.isIntersecting).toString(); },
      { rootMargin: '-20px 0px 0px 0px', threshold: 0 }
    );
    io.observe(document.body);
  }

  /* ── Hamburger ─────────────────────────────────────────────────── */
  const toggle = document.querySelector('.nav-toggle');
  const links = document.querySelector('.nav-links');
  if (toggle && links) {
    toggle.addEventListener('click', () => {
      const open = links.classList.toggle('open');
      toggle.setAttribute('aria-expanded', open);
      toggle.textContent = open ? '✕' : 'Menu';
    });
    links.querySelectorAll('a').forEach(a => {
      a.addEventListener('click', () => {
        links.classList.remove('open');
        toggle.setAttribute('aria-expanded', 'false');
        toggle.textContent = 'Menu';
      });
    });
  }

  /* ── Copy buttons ──────────────────────────────────────────────── */
  document.querySelectorAll('.codewrap').forEach(wrap => {
    const btn = wrap.querySelector('.copy-btn');
    const pre = wrap.querySelector('pre');
    if (!btn || !pre) return;
    btn.addEventListener('click', () => {
      const text = pre.textContent.replace(/^\$ /gm, '');
      navigator.clipboard.writeText(text).then(() => {
        btn.dataset.copied = 'true';
        btn.textContent = 'Copied';
        setTimeout(() => { btn.dataset.copied = 'false'; btn.textContent = 'Copy'; }, 1500);
      });
    });
  });

  /* ── Scroll reveal ─────────────────────────────────────────────── */
  const reveals = document.querySelectorAll('.reveal, .reveal-stagger');
  if (reveals.length) {
    const ro = new IntersectionObserver((entries) => {
      entries.forEach(e => {
        if (e.isIntersecting) {
          e.target.classList.add('visible');
          ro.unobserve(e.target);
        }
      });
    }, { threshold: 0.1, rootMargin: '0px 0px -40px 0px' });
    reveals.forEach(el => ro.observe(el));
  }

  /* ── Tab groups ────────────────────────────────────────────────── */
  document.querySelectorAll('.tabs').forEach(tabBar => {
    const buttons = tabBar.querySelectorAll('.tab-btn');
    buttons.forEach(btn => {
      btn.addEventListener('click', () => {
        const target = btn.dataset.tab;
        // Deactivate siblings
        buttons.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        // Find panels: try data-tab id match first, then index fallback
        const panelContainer = tabBar.nextElementSibling;
        if (!panelContainer) return;
        const panels = panelContainer.querySelectorAll('.tab-panel');
        // Try id-based match
        const byId = panelContainer.querySelector(`#tab-${target}`);
        if (byId) {
          panels.forEach(p => p.classList.remove('active'));
          byId.classList.add('active');
        } else {
          // Index fallback
          const idx = Array.from(buttons).indexOf(btn);
          panels.forEach(p => p.classList.remove('active'));
          if (panels[idx]) panels[idx].classList.add('active');
        }
      });
    });
  });

  /* ── Active doc nav link ───────────────────────────────────────── */
  const docNav = document.querySelector('.docs-nav');
  if (docNav) {
    const path = location.pathname.split('/').pop();
    docNav.querySelectorAll('a').forEach(a => {
      if (a.getAttribute('href') === path || a.getAttribute('href') === './' + path) {
        a.classList.add('active');
      }
    });
  }

  /* ── Docs mobile toggle ────────────────────────────────────────── */
  const docToggle = document.querySelector('.docs-toggle');
  const docSidebar = document.querySelector('.docs-nav');
  if (docToggle && docSidebar) {
    docToggle.addEventListener('click', () => docSidebar.classList.toggle('open'));
  }

  /* ── Active page in nav ────────────────────────────────────────── */
  const current = location.pathname.split('/').pop() || 'index.html';
  document.querySelectorAll('.nav-links a').forEach(a => {
    const href = a.getAttribute('href') || '';
    if (href === current || href === './' + current) {
      a.setAttribute('aria-current', 'page');
    }
  });

})();
